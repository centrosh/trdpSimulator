#include "trdp_simulator/simulation/ScenarioLoader.hpp"

#include "trdp_simulator/device/DeviceProfileRepository.hpp"

#include <stdexcept>

namespace trdp::simulation {

ScenarioLoader::ScenarioLoader(device::DeviceProfileRepository &repository, ScenarioSchemaValidator &validator,
                               std::filesystem::path scenarioRoot)
    : m_repository(repository), m_validator(validator), m_scenarioRoot(std::move(scenarioRoot)) {
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
    m_validator.validate(path);
    return ScenarioParser::parse(path, m_repository);
}

Scenario ScenarioLoader::loadFromFile(const std::filesystem::path &path) const {
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("Scenario file not found: " + path.string());
    }
    m_validator.validate(path);
    return ScenarioParser::parse(path, m_repository);
}

} // namespace trdp::simulation

