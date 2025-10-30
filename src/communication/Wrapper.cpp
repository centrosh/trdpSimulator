#include "trdp_simulator/communication/Wrapper.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
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
} // namespace

Wrapper::Wrapper(std::string endpoint)
    : m_endpoint(std::move(endpoint)) {}

void Wrapper::open() {
    if (m_open) {
        throw std::runtime_error("TRDP connection already open");
    }
    m_open = true;
    m_telemetry.emplace_back(makeTimestamp() + " | open -> " + m_endpoint);
}

void Wrapper::close() {
    if (!m_open) {
        throw std::runtime_error("TRDP connection not open");
    }
    m_open = false;
    m_telemetry.emplace_back(makeTimestamp() + " | close");
}

void Wrapper::publishProcessData(std::string_view label) {
    if (!m_open) {
        throw std::runtime_error("Cannot publish PD telegram: connection closed");
    }
    m_telemetry.emplace_back(makeTimestamp() + " | pd -> " + std::string{label});
}

void Wrapper::sendMessageData(std::string_view label) {
    if (!m_open) {
        throw std::runtime_error("Cannot send MD telegram: connection closed");
    }
    m_telemetry.emplace_back(makeTimestamp() + " | md -> " + std::string{label});
}

bool Wrapper::isOpen() const noexcept { return m_open; }

const std::vector<std::string> &Wrapper::telemetry() const noexcept { return m_telemetry; }

} // namespace trdp::communication
