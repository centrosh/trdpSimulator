#pragma once

#include "trdp_simulator/simulation/ScenarioParser.hpp"

#include <filesystem>
#include <set>
#include <string>
#include <vector>

namespace trdp::simulation {

class ScenarioSchemaValidator {
public:
    explicit ScenarioSchemaValidator(std::filesystem::path schemaPath);

    void validate(const std::filesystem::path &scenarioPath) const;

    [[nodiscard]] const std::filesystem::path &schemaPath() const noexcept { return m_schemaPath; }

private:
    std::filesystem::path m_schemaPath;
    std::set<std::string> m_requiredScenarioFields;
    std::set<std::string> m_allowedScenarioFields;
    std::set<std::string> m_requiredEventFields;
    std::set<std::string> m_allowedEventFields;
    std::set<std::string> m_eventTypeValues;
    std::set<std::string> m_numericEventFields;

    void loadSchema();
};

} // namespace trdp::simulation

