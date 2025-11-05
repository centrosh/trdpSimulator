#include "trdp_simulator/simulation/ScenarioYaml.hpp"

#include <sstream>
#include <stdexcept>

namespace trdp::simulation::scenario_yaml {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
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
        payload.reserve(hex.size() / 2);
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

std::string describeEvent(const ScenarioEvent &event) {
    std::ostringstream oss;
    oss << (event.type == ScenarioEvent::Type::ProcessData ? "pd" : "md");
    oss << "::" << event.label;
    oss << "::comId=" << event.comId;
    oss << "::dataset=" << event.datasetId;
    oss << "::bytes=" << event.payload.size();
    oss << "::delayMs=" << event.delay.count();
    return oss.str();
}

} // namespace trdp::simulation::scenario_yaml

