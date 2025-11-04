#pragma once

#include "trdp_simulator/communication/Diagnostics.hpp"
#include "trdp_simulator/communication/StackAdapter.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace trdp::communication {

class Wrapper {
public:
    explicit Wrapper(std::string endpoint = "localhost", std::shared_ptr<StackAdapter> adapter = {});

    void open();
    void close();

    void publishProcessData(std::string_view label);
    void sendMessageData(std::string_view label);

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] const std::vector<std::string> &telemetry() const noexcept;
    [[nodiscard]] const std::vector<DiagnosticEvent> &diagnostics() const noexcept;

private:
    void recordInfo(std::string message);
    void recordError(std::string message);

    std::string m_endpoint;
    std::shared_ptr<StackAdapter> m_adapter;
    bool m_open{false};
    std::vector<std::string> m_telemetry;
    std::vector<DiagnosticEvent> m_diagnostics;
};

} // namespace trdp::communication
