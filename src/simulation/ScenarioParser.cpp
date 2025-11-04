#include "trdp_simulator/simulation/ScenarioParser.hpp"

#include "trdp_simulator/device/DeviceProfileRepository.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace trdp::simulation {
namespace {

[[nodiscard]] std::string trim(const std::string &value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

ScenarioEvent::Type parseType(const std::string &token) {
    if (token == "pd") {
        return ScenarioEvent::Type::ProcessData;
    }
    if (token == "md") {
        return ScenarioEvent::Type::MessageData;
    }
    throw ScenarioValidationError{"Unknown event type: " + token};
}

std::vector<std::uint8_t> parsePayload(const std::string &value) {
    if (value.empty()) {
        return {};
    }
    if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        if ((value.size() - 2) % 2 != 0) {
            throw ScenarioValidationError{"Hex payload must contain an even number of characters"};
        }
        std::vector<std::uint8_t> payload;
        const auto hex = value.substr(2);
        for (std::size_t i = 0; i < hex.size(); i += 2) {
            const auto byteStr = hex.substr(i, 2);
            const auto byte = static_cast<std::uint8_t>(std::stoul(byteStr, nullptr, 16));
            payload.push_back(byte);
        }
        return payload;
    }
    return std::vector<std::uint8_t>(value.begin(), value.end());
}

std::chrono::milliseconds parseDelay(const std::string &value) {
    if (value.empty()) {
        return std::chrono::milliseconds{0};
    }
    return std::chrono::milliseconds{std::stoll(value)};
}

struct EventState {
    ScenarioEvent event{};
    bool typeSet{false};
    bool labelSet{false};
};

void applyField(EventState &state, const std::string &key, const std::string &value) {
    if (key == "type") {
        state.event.type = parseType(value);
        state.typeSet = true;
    } else if (key == "label") {
        state.event.label = value;
        state.labelSet = true;
    } else if (key == "com_id") {
        state.event.comId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "dataset_id") {
        state.event.datasetId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "payload") {
        state.event.payload = parsePayload(value);
    } else if (key == "delay_ms") {
        state.event.delay = parseDelay(value);
    } else {
        throw ScenarioValidationError{"Unknown event field: " + key};
    }
}

std::pair<std::string, std::string> parseKeyValue(const std::string &line) {
    const auto pos = line.find(':');
    if (pos == std::string::npos) {
        throw ScenarioValidationError{"Invalid line (missing ':'): " + line};
    }
    auto key = trim(line.substr(0, pos));
    auto value = trim(line.substr(pos + 1));
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return {std::move(key), std::move(value)};
}

void finaliseEvent(const EventState &state, Scenario &scenario) {
    if (!state.typeSet) {
        throw ScenarioValidationError{"Scenario event is missing a type"};
    }
    if (!state.labelSet) {
        throw ScenarioValidationError{"Scenario event is missing a label"};
    }
    scenario.events.push_back(state.event);
}

} // namespace

Scenario ScenarioParser::parse(const std::filesystem::path &path, device::DeviceProfileRepository &repository) {
    if (!std::filesystem::exists(path)) {
        throw ScenarioValidationError{"Scenario file not found: " + path.string()};
    }

    std::ifstream stream{path};
    if (!stream) {
        throw ScenarioValidationError{"Failed to open scenario file: " + path.string()};
    }

    Scenario scenario{};
    scenario.id = path.stem().string();

    bool inEvents = false;
    bool eventActive = false;
    EventState current{};

    std::string rawLine;
    while (std::getline(stream, rawLine)) {
        const auto trimmed = trim(rawLine);
        if (trimmed.empty() || trimmed.starts_with('#')) {
            continue;
        }

        if (trimmed == "events:") {
            inEvents = true;
            continue;
        }

        if (!inEvents) {
            const auto [key, value] = parseKeyValue(trimmed);
            if (key == "scenario") {
                if (value.empty()) {
                    throw ScenarioValidationError{"Scenario id cannot be empty"};
                }
                scenario.id = value;
            } else if (key == "device") {
                if (value.empty()) {
                    throw ScenarioValidationError{"Scenario device cannot be empty"};
                }
                scenario.deviceProfileId = value;
            } else {
                throw ScenarioValidationError{"Unknown scenario field: " + key};
            }
            continue;
        }

        if (trimmed.rfind("-", 0) == 0) {
            if (eventActive) {
                finaliseEvent(current, scenario);
                current = EventState{};
            }
            eventActive = true;
            const auto afterDash = trim(trimmed.substr(1));
            if (!afterDash.empty()) {
                const auto [key, value] = parseKeyValue(afterDash);
                applyField(current, key, value);
            }
            continue;
        }

        if (!eventActive) {
            throw ScenarioValidationError{"Event field defined outside of list: " + trimmed};
        }

        const auto [key, value] = parseKeyValue(trimmed);
        applyField(current, key, value);
    }

    if (eventActive) {
        finaliseEvent(current, scenario);
    }

    if (scenario.deviceProfileId.empty()) {
        throw ScenarioValidationError{"Scenario does not reference a device profile"};
    }

    if (!repository.exists(scenario.deviceProfileId)) {
        throw ScenarioValidationError{"Scenario references unknown device profile: " + scenario.deviceProfileId};
    }

    if (scenario.events.empty()) {
        throw ScenarioValidationError{"Scenario does not contain any events"};
    }

    return scenario;
}

} // namespace trdp::simulation

