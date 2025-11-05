#include "trdp_simulator/simulation/ScenarioParser.hpp"

#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/simulation/ScenarioYaml.hpp"

#include <filesystem>
#include <fstream>

namespace trdp::simulation {
namespace {

struct EventState {
    ScenarioEvent event{};
    bool typeSet{false};
    bool labelSet{false};
};

void applyField(EventState &state, const std::string &key, const std::string &value) {
    if (key == "type") {
        state.event.type = scenario_yaml::parseType(value);
        state.typeSet = true;
    } else if (key == "label") {
        state.event.label = value;
        state.labelSet = true;
    } else if (key == "com_id") {
        state.event.comId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "dataset_id") {
        state.event.datasetId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "payload") {
        state.event.payload = scenario_yaml::parsePayload(value);
    } else if (key == "delay_ms") {
        state.event.delay = scenario_yaml::parseDelay(value);
    } else {
        throw ScenarioValidationError{"Unknown event field: " + key};
    }
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
        const auto trimmed = scenario_yaml::trim(rawLine);
        if (trimmed.empty() || trimmed.starts_with('#')) {
            continue;
        }

        if (trimmed == "events:") {
            inEvents = true;
            continue;
        }

        if (!inEvents) {
            const auto [key, value] = scenario_yaml::parseKeyValue(trimmed);
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
            const auto afterDash = scenario_yaml::trim(trimmed.substr(1));
            if (!afterDash.empty()) {
                const auto [key, value] = scenario_yaml::parseKeyValue(afterDash);
                applyField(current, key, value);
            }
            continue;
        }

        if (!eventActive) {
            throw ScenarioValidationError{"Event field defined outside of list: " + trimmed};
        }

        const auto [key, value] = scenario_yaml::parseKeyValue(trimmed);
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

