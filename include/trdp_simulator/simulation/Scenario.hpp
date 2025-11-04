#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace trdp::simulation {

struct ScenarioEvent {
    enum class Type {
        ProcessData,
        MessageData,
    };

    Type type{Type::ProcessData};
    std::string label;
    std::uint32_t comId{0};
    std::uint32_t datasetId{0};
    std::vector<std::uint8_t> payload;
    std::chrono::milliseconds delay{0};
};

struct Scenario {
    std::string id;
    std::string deviceProfileId;
    std::vector<ScenarioEvent> events;
};

} // namespace trdp::simulation

