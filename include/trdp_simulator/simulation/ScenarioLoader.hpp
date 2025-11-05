#pragma once

#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/simulation/Scenario.hpp"
#include "trdp_simulator/simulation/ScenarioParser.hpp"
#include "trdp_simulator/simulation/ScenarioSchemaValidator.hpp"

#include <filesystem>

namespace trdp::simulation {

class ScenarioLoader {
public:
    ScenarioLoader(device::DeviceProfileRepository &repository, ScenarioSchemaValidator &validator,
                   std::filesystem::path scenarioRoot);

    [[nodiscard]] Scenario load(const std::string &scenarioId) const;
    [[nodiscard]] Scenario loadFromFile(const std::filesystem::path &path) const;

private:
    device::DeviceProfileRepository &m_repository;
    ScenarioSchemaValidator &m_validator;
    std::filesystem::path m_scenarioRoot;
};

} // namespace trdp::simulation

