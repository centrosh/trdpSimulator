#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace trdp::communication {

class Wrapper {
public:
    explicit Wrapper(std::string endpoint = "localhost");

    void open();
    void close();

    void publishProcessData(std::string_view label);
    void sendMessageData(std::string_view label);

    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] const std::vector<std::string> &telemetry() const noexcept;

private:
    std::string m_endpoint;
    bool m_open{false};
    std::vector<std::string> m_telemetry;
};

} // namespace trdp::communication
