#pragma once

#include "trdp_simulator/simulation/Scenario.hpp"
#include "trdp_simulator/simulation/ScenarioParser.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace trdp::simulation::scenario_yaml {

[[nodiscard]] std::string trim(std::string value);
[[nodiscard]] std::pair<std::string, std::string> parseKeyValue(const std::string &line);
[[nodiscard]] ScenarioEvent::Type parseType(const std::string &token);
[[nodiscard]] std::vector<std::uint8_t> parsePayload(const std::string &value);
[[nodiscard]] std::chrono::milliseconds parseDelay(const std::string &value);
[[nodiscard]] std::string describeEvent(const ScenarioEvent &event);

} // namespace trdp::simulation::scenario_yaml

