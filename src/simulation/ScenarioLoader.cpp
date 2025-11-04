#include "trdp_simulator/simulation/ScenarioLoader.hpp"

#include "trdp_simulator/device/DeviceProfileRepository.hpp"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

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
    throw std::invalid_argument("Unknown event type: " + token);
}

std::vector<std::uint8_t> parsePayload(const std::string &value) {
    if (value.empty()) {
        return {};
    }
    if (value.size() > 2 && value[0] == '0' && (value[1] == 'x' || value[1] == 'X')) {
        std::vector<std::uint8_t> payload;
        const auto hex = value.substr(2);
        if (hex.size() % 2 != 0) {
            throw std::invalid_argument("Hex payload must contain an even number of characters");
        }
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

void applyField(ScenarioEvent &event, const std::string &key, const std::string &value) {
    if (key == "type") {
        event.type = parseType(value);
    } else if (key == "label") {
        event.label = value;
    } else if (key == "com_id") {
        event.comId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "dataset_id") {
        event.datasetId = static_cast<std::uint32_t>(std::stoul(value));
    } else if (key == "payload") {
        event.payload = parsePayload(value);
    } else if (key == "delay_ms") {
        event.delay = parseDelay(value);
    }
}

std::pair<std::string, std::string> parseKeyValue(const std::string &line) {
    const auto pos = line.find(':');
    if (pos == std::string::npos) {
        throw std::invalid_argument("Invalid scenario line: " + line);
    }
    auto key = trim(line.substr(0, pos));
    auto value = trim(line.substr(pos + 1));
    if (!value.empty() && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return {std::move(key), std::move(value)};
}

} // namespace

ScenarioLoader::ScenarioLoader(device::DeviceProfileRepository &repository, std::filesystem::path scenarioRoot)
    : m_repository(repository), m_scenarioRoot(std::move(scenarioRoot)) {
    if (!m_scenarioRoot.empty()) {
        std::filesystem::create_directories(m_scenarioRoot);
    }
}

Scenario ScenarioLoader::load(const std::string &scenarioId) const {
    if (m_scenarioRoot.empty()) {
        throw std::runtime_error("Scenario root directory is not configured");
    }
    const auto path = m_scenarioRoot / (scenarioId + ".yaml");
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Scenario file not found: " + path.string());
    }
    return parseScenario(path);
}

Scenario ScenarioLoader::loadFromFile(const std::filesystem::path &path) const {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Scenario file not found: " + path.string());
    }
    return parseScenario(path);
}

Scenario ScenarioLoader::parseScenario(const std::filesystem::path &path) const {
    std::ifstream stream{path};
    if (!stream) {
        throw std::runtime_error("Failed to open scenario file: " + path.string());
    }

    Scenario scenario{};
    scenario.id = path.stem().string();

    bool inEvents = false;
    bool eventActive = false;
    ScenarioEvent currentEvent{};

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
                scenario.id = value;
            } else if (key == "device") {
                scenario.deviceProfileId = value;
            }
            continue;
        }

        if (trimmed.rfind("-", 0) == 0) {
            if (eventActive) {
                scenario.events.push_back(currentEvent);
                currentEvent = ScenarioEvent{};
            }
            eventActive = true;
            const auto afterDash = trim(trimmed.substr(1));
            if (!afterDash.empty()) {
                const auto [key, value] = parseKeyValue(afterDash);
                applyField(currentEvent, key, value);
            }
            continue;
        }

        if (!eventActive) {
            throw std::invalid_argument("Event field defined outside of list: " + trimmed);
        }

        const auto [key, value] = parseKeyValue(trimmed);
        applyField(currentEvent, key, value);
    }

    if (eventActive) {
        scenario.events.push_back(currentEvent);
    }

    if (scenario.deviceProfileId.empty()) {
        throw std::invalid_argument("Scenario does not reference a device profile");
    }

    if (!m_repository.exists(scenario.deviceProfileId)) {
        throw std::runtime_error("Scenario references unknown device profile: " + scenario.deviceProfileId);
    }

    return scenario;
}

} // namespace trdp::simulation

