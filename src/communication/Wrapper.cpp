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

    void publishProcessData(std::string_view /*label*/) override { ensureOpen("publishProcessData"); }
    void sendMessageData(std::string_view /*label*/) override { ensureOpen("sendMessageData"); }

private:
    void ensureOpen(std::string_view operation) const {
        if (!m_open) {
            throw TrdpError(std::string(operation) + " called without open session", 1003, std::string(operation));
        }
    }

    bool m_open{false};
    std::string m_endpoint;
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

void Wrapper::publishProcessData(std::string_view label) {
    if (!m_open) {
        throw std::runtime_error("Cannot publish PD telegram: connection closed");
    }
    try {
        m_adapter->publishProcessData(label);
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
    recordInfo("pd -> " + std::string{label});
}

void Wrapper::sendMessageData(std::string_view label) {
    if (!m_open) {
        throw std::runtime_error("Cannot send MD telegram: connection closed");
    }
    try {
        m_adapter->sendMessageData(label);
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
    recordInfo("md -> " + std::string{label});
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

} // namespace trdp::communication
