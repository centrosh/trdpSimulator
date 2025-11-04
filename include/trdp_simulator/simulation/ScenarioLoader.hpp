#pragma once

#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/simulation/Scenario.hpp"

#include <filesystem>

namespace trdp::simulation {

class ScenarioLoader {
public:
    ScenarioLoader(device::DeviceProfileRepository &repository, std::filesystem::path scenarioRoot);

    [[nodiscard]] Scenario load(const std::string &scenarioId) const;
    [[nodiscard]] Scenario loadFromFile(const std::filesystem::path &path) const;

private:
    device::DeviceProfileRepository &m_repository;
    std::filesystem::path m_scenarioRoot;

    Scenario parseScenario(const std::filesystem::path &path) const;
};

} // namespace trdp::simulation

