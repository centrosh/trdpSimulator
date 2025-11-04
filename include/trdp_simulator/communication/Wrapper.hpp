#pragma once

#include "trdp_simulator/communication/Diagnostics.hpp"
#include "trdp_simulator/communication/StackAdapter.hpp"
#include "trdp_simulator/communication/Types.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace trdp::communication {

class Wrapper {
public:
    using ProcessDataCallback = std::function<void(const ProcessDataMessage &)>;
    using MessageDataCallback = std::function<void(const MessageDataMessage &)>;

    explicit Wrapper(std::string endpoint = "localhost", std::shared_ptr<StackAdapter> adapter = {});

    void open();
    void close();

    void registerProcessDataHandler(ProcessDataCallback callback);
    void registerMessageDataHandler(MessageDataCallback callback);

    void publishProcessData(const ProcessDataMessage &message);
    MessageDataAck sendMessageData(const MessageDataMessage &message);

    void poll();

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] const std::vector<std::string> &telemetry() const noexcept;
    [[nodiscard]] const std::vector<DiagnosticEvent> &diagnostics() const noexcept;

private:
    void recordInfo(std::string message);
    void recordError(std::string message);
    void handleProcessData(const ProcessDataMessage &message);
    void handleMessageData(const MessageDataMessage &message);

    std::string m_endpoint;
    std::shared_ptr<StackAdapter> m_adapter;
    bool m_open{false};
    std::vector<std::string> m_telemetry;
    std::vector<DiagnosticEvent> m_diagnostics;
    ProcessDataCallback m_processDataCallback;
    MessageDataCallback m_messageDataCallback;
};

} // namespace trdp::communication
