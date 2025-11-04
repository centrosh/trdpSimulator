#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace trdp::communication {

struct ProcessDataMessage {
    std::string label;
    std::uint32_t comId{};
    std::uint32_t datasetId{};
    std::vector<std::uint8_t> payload;
};

struct MessageDataMessage {
    std::string label;
    std::uint32_t comId{};
    std::uint32_t datasetId{};
    std::vector<std::uint8_t> payload;
};

enum class MessageDataStatus {
    Delivered,
    Timeout,
    Failed,
};

struct MessageDataAck {
    MessageDataStatus status{MessageDataStatus::Delivered};
    std::string detail;
};

} // namespace trdp::communication
