#include "trdp_simulator/communication/Wrapper.hpp"

#include "trdp_simulator/communication/TrdpError.hpp"

#include <chrono>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace trdp::communication {

namespace {
[[nodiscard]] std::string makeTimestamp() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto time = clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

[[nodiscard]] std::string formatPdMessage(const ProcessDataMessage &message) {
    std::ostringstream oss;
    oss << message.label << " (comId=" << message.comId << ", dataset=" << message.datasetId
        << ", bytes=" << message.payload.size() << ')';
    return oss.str();
}

[[nodiscard]] std::string formatMdMessage(const MessageDataMessage &message) {
    std::ostringstream oss;
    oss << message.label << " (comId=" << message.comId << ", dataset=" << message.datasetId
        << ", bytes=" << message.payload.size() << ')';
    return oss.str();
}

[[nodiscard]] std::string formatAck(const MessageDataAck &ack) {
    switch (ack.status) {
    case MessageDataStatus::Delivered:
        return "delivered" + (ack.detail.empty() ? std::string{} : " - " + ack.detail);
    case MessageDataStatus::Timeout:
        return "timeout" + (ack.detail.empty() ? std::string{} : " - " + ack.detail);
    case MessageDataStatus::Failed:
        return "failed" + (ack.detail.empty() ? std::string{} : " - " + ack.detail);
    }
    return "unknown";
}

class DummyStackAdapter final : public StackAdapter {
public:
    void openSession(const std::string &endpoint) override {
        if (m_open) {
            throw TrdpError("Session already open", 1001, endpoint);
        }
        m_open = true;
        m_endpoint = endpoint;
    }

    void closeSession() override {
        if (!m_open) {
            throw TrdpError("Session already closed", 1002, m_endpoint);
        }
        m_open = false;
    }

    void registerProcessDataHandler(ProcessDataHandler handler) override { m_pdHandler = std::move(handler); }

    void registerMessageDataHandler(MessageDataHandler handler) override { m_mdHandler = std::move(handler); }

    void publishProcessData(const ProcessDataMessage &message) override {
        ensureOpen("publishProcessData");
        if (m_pdHandler) {
            m_pdHandler(message);
        }
    }

    MessageDataAck sendMessageData(const MessageDataMessage &message) override {
        ensureOpen("sendMessageData");
        if (m_mdHandler) {
            m_mdHandler(message);
        }
        return MessageDataAck{MessageDataStatus::Delivered, "loopback"};
    }

    void poll() override { /* no-op loopback */ }

private:
    void ensureOpen(std::string_view operation) const {
        if (!m_open) {
            throw TrdpError(std::string(operation) + " called without open session", 1003, std::string(operation));
        }
    }

    bool m_open{false};
    std::string m_endpoint;
    ProcessDataHandler m_pdHandler;
    MessageDataHandler m_mdHandler;
};

[[nodiscard]] std::shared_ptr<StackAdapter> makeDefaultAdapter() {
    return std::make_shared<DummyStackAdapter>();
}

void appendTelemetry(std::vector<std::string> &buffer, const std::string &timestamp, const std::string &message, bool error) {
    if (error) {
        buffer.emplace_back(timestamp + " | error -> " + message);
    } else {
        buffer.emplace_back(timestamp + " | " + message);
    }
}

} // namespace

Wrapper::Wrapper(std::string endpoint, std::shared_ptr<StackAdapter> adapter)
    : m_endpoint(std::move(endpoint)), m_adapter(adapter ? std::move(adapter) : makeDefaultAdapter()) {
    if (!m_adapter) {
        throw std::invalid_argument("Stack adapter cannot be null");
    }

    m_adapter->registerProcessDataHandler([this](const ProcessDataMessage &message) { handleProcessData(message); });
    m_adapter->registerMessageDataHandler([this](const MessageDataMessage &message) { handleMessageData(message); });
}

void Wrapper::open() {
    if (m_open) {
        throw std::runtime_error("TRDP connection already open");
    }
    try {
        m_adapter->openSession(m_endpoint);
    } catch (const TrdpError &error) {
        std::ostringstream oss;
        oss << "open failure (code " << error.errorCode() << ")";
        if (!error.context().empty()) {
            oss << " context=" << error.context();
        }
        oss << ": " << error.what();
        recordError(oss.str());
        throw;
    }
    m_open = true;
    recordInfo("open -> " + m_endpoint);
}

void Wrapper::close() {
    if (!m_open) {
        throw std::runtime_error("TRDP connection not open");
    }
    try {
        m_adapter->closeSession();
    } catch (const TrdpError &error) {
        std::ostringstream oss;
        oss << "close failure (code " << error.errorCode() << ")";
        if (!error.context().empty()) {
            oss << " context=" << error.context();
        }
        oss << ": " << error.what();
        recordError(oss.str());
        throw;
    }
    m_open = false;
    recordInfo("close");
}

void Wrapper::registerProcessDataHandler(ProcessDataCallback callback) { m_processDataCallback = std::move(callback); }

void Wrapper::registerMessageDataHandler(MessageDataCallback callback) { m_messageDataCallback = std::move(callback); }

void Wrapper::publishProcessData(const ProcessDataMessage &message) {
    if (!m_open) {
        throw std::runtime_error("Cannot publish PD telegram: connection closed");
    }
    try {
        m_adapter->publishProcessData(message);
    } catch (const TrdpError &error) {
        std::ostringstream oss;
        oss << "pd failure (code " << error.errorCode() << ")";
        if (!error.context().empty()) {
            oss << " context=" << error.context();
        }
        oss << ": " << error.what();
        recordError(oss.str());
        throw;
    }
    recordInfo("pd -> " + formatPdMessage(message));
}

MessageDataAck Wrapper::sendMessageData(const MessageDataMessage &message) {
    if (!m_open) {
        throw std::runtime_error("Cannot send MD telegram: connection closed");
    }
    MessageDataAck ack;
    try {
        ack = m_adapter->sendMessageData(message);
    } catch (const TrdpError &error) {
        std::ostringstream oss;
        oss << "md failure (code " << error.errorCode() << ")";
        if (!error.context().empty()) {
            oss << " context=" << error.context();
        }
        oss << ": " << error.what();
        recordError(oss.str());
        throw;
    }
    std::ostringstream oss;
    oss << "md -> " << formatMdMessage(message) << " | " << formatAck(ack);
    recordInfo(oss.str());
    return ack;
}

void Wrapper::poll() {
    try {
        m_adapter->poll();
    } catch (const TrdpError &error) {
        std::ostringstream oss;
        oss << "poll failure (code " << error.errorCode() << ")";
        if (!error.context().empty()) {
            oss << " context=" << error.context();
        }
        oss << ": " << error.what();
        recordError(oss.str());
        throw;
    }
}

bool Wrapper::isOpen() const noexcept { return m_open; }

const std::vector<std::string> &Wrapper::telemetry() const noexcept { return m_telemetry; }

const std::vector<DiagnosticEvent> &Wrapper::diagnostics() const noexcept { return m_diagnostics; }

void Wrapper::recordInfo(std::string message) {
    const auto timestamp = makeTimestamp();
    m_diagnostics.push_back(DiagnosticEvent{timestamp, DiagnosticEvent::Level::Info, message});
    appendTelemetry(m_telemetry, timestamp, message, false);
}

void Wrapper::recordError(std::string message) {
    const auto timestamp = makeTimestamp();
    m_diagnostics.push_back(DiagnosticEvent{timestamp, DiagnosticEvent::Level::Error, message});
    appendTelemetry(m_telemetry, timestamp, message, true);
}

void Wrapper::handleProcessData(const ProcessDataMessage &message) {
    recordInfo("pd <- " + formatPdMessage(message));
    if (m_processDataCallback) {
        m_processDataCallback(message);
    }
}

void Wrapper::handleMessageData(const MessageDataMessage &message) {
    recordInfo("md <- " + formatMdMessage(message));
    if (m_messageDataCallback) {
        m_messageDataCallback(message);
    }
}

} // namespace trdp::communication
