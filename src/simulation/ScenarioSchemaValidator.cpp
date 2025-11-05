#include "trdp_simulator/simulation/ScenarioSchemaValidator.hpp"

#include "trdp_simulator/simulation/ScenarioYaml.hpp"

#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace trdp::simulation {
namespace {

[[nodiscard]] std::vector<std::string> splitList(const std::string &value) {
    std::vector<std::string> tokens;
    std::stringstream ss{value};
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = scenario_yaml::trim(token);
        if (!token.empty()) {
            tokens.emplace_back(std::move(token));
        }
    }
    return tokens;
}

void ensureRequiredFields(const std::set<std::string> &required, const std::set<std::string> &present,
                          const std::string &context) {
    for (const auto &field : required) {
        if (!present.contains(field)) {
            throw ScenarioValidationError{"Missing required " + context + " field: " + field};
        }
    }
}

void validateEventField(const std::string &key, const std::string &value, const std::set<std::string> &numericFields,
                        const std::set<std::string> &allowedTypeValues) {
    if (key == "type") {
        if (!allowedTypeValues.contains(value)) {
            std::ostringstream oss;
            oss << "Event type '" << value << "' not permitted";
            throw ScenarioValidationError{oss.str()};
        }
        (void)scenario_yaml::parseType(value);
        return;
    }
    if (numericFields.contains(key)) {
        if (value.empty()) {
            throw ScenarioValidationError{"Numeric event field '" + key + "' cannot be empty"};
        }
        for (char ch : value) {
            if (!std::isdigit(static_cast<unsigned char>(ch))) {
                throw ScenarioValidationError{"Numeric event field '" + key + "' contains non-digit characters"};
            }
        }
        return;
    }
    if (key == "payload") {
        (void)scenario_yaml::parsePayload(value);
        return;
    }
    (void)scenario_yaml::parseKeyValue(key + ": " + value);
}

} // namespace

ScenarioSchemaValidator::ScenarioSchemaValidator(std::filesystem::path schemaPath)
    : m_schemaPath(std::move(schemaPath)) {
    if (m_schemaPath.empty()) {
        throw std::invalid_argument("Scenario schema path cannot be empty");
    }
    loadSchema();
}

void ScenarioSchemaValidator::loadSchema() {
    if (!std::filesystem::exists(m_schemaPath)) {
        throw std::runtime_error("Scenario schema not found: " + m_schemaPath.string());
    }
    std::ifstream stream{m_schemaPath};
    if (!stream) {
        throw std::runtime_error("Failed to open scenario schema: " + m_schemaPath.string());
    }

    std::string line;
    while (std::getline(stream, line)) {
        const auto trimmed = scenario_yaml::trim(line);
        if (trimmed.empty() || trimmed.starts_with('#')) {
            continue;
        }
        const auto [key, rawValue] = scenario_yaml::parseKeyValue(trimmed);
        const auto values = splitList(rawValue);
        if (key == "required_scenario_fields") {
            m_requiredScenarioFields = std::set<std::string>(values.begin(), values.end());
        } else if (key == "allowed_scenario_fields") {
            m_allowedScenarioFields = std::set<std::string>(values.begin(), values.end());
        } else if (key == "required_event_fields") {
            m_requiredEventFields = std::set<std::string>(values.begin(), values.end());
        } else if (key == "allowed_event_fields") {
            m_allowedEventFields = std::set<std::string>(values.begin(), values.end());
        } else if (key == "enum_event_type") {
            m_eventTypeValues = std::set<std::string>(values.begin(), values.end());
        } else if (key == "numeric_event_fields") {
            m_numericEventFields = std::set<std::string>(values.begin(), values.end());
        }
    }

    if (m_allowedScenarioFields.empty()) {
        m_allowedScenarioFields = {"scenario", "device"};
    }
    if (m_requiredScenarioFields.empty()) {
        m_requiredScenarioFields = {"scenario", "device"};
    }
    if (m_allowedEventFields.empty()) {
        m_allowedEventFields = {"type", "label", "com_id", "dataset_id", "payload", "delay_ms"};
    }
    if (m_requiredEventFields.empty()) {
        m_requiredEventFields = {"type", "label"};
    }
    if (m_eventTypeValues.empty()) {
        m_eventTypeValues = {"pd", "md"};
    }
}

void ScenarioSchemaValidator::validate(const std::filesystem::path &scenarioPath) const {
    if (!std::filesystem::exists(scenarioPath)) {
        throw ScenarioValidationError{"Scenario file not found: " + scenarioPath.string()};
    }

    std::ifstream stream{scenarioPath};
    if (!stream) {
        throw ScenarioValidationError{"Failed to open scenario file: " + scenarioPath.string()};
    }

    bool inEvents = false;
    bool eventActive = false;
    std::set<std::string> scenarioFields;
    std::set<std::string> eventFields;
    std::size_t eventCount = 0;

    std::regex scenarioIdPattern{"^[A-Za-z0-9_-]+$"};

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
            if (!m_allowedScenarioFields.contains(key) && key != "events") {
                throw ScenarioValidationError{"Unknown scenario field: " + key};
            }
            scenarioFields.insert(key);
            if (key == "scenario") {
                if (value.empty()) {
                    throw ScenarioValidationError{"Scenario id cannot be empty"};
                }
                if (!std::regex_match(value, scenarioIdPattern)) {
                    throw ScenarioValidationError{"Scenario id contains invalid characters"};
                }
            } else if (key == "device") {
                if (value.empty()) {
                    throw ScenarioValidationError{"Scenario device cannot be empty"};
                }
            }
            continue;
        }

        if (trimmed.rfind('-', 0) == 0) {
            if (eventActive) {
                ensureRequiredFields(m_requiredEventFields, eventFields, "event");
                eventFields.clear();
            }
            eventActive = true;
            ++eventCount;
            const auto afterDash = scenario_yaml::trim(trimmed.substr(1));
            if (!afterDash.empty()) {
                const auto [key, value] = scenario_yaml::parseKeyValue(afterDash);
                if (!m_allowedEventFields.contains(key)) {
                    throw ScenarioValidationError{"Unknown event field: " + key};
                }
                validateEventField(key, value, m_numericEventFields, m_eventTypeValues);
                eventFields.insert(key);
            }
            continue;
        }

        if (!eventActive) {
            throw ScenarioValidationError{"Event field defined outside of list: " + trimmed};
        }

        const auto [key, value] = scenario_yaml::parseKeyValue(trimmed);
        if (!m_allowedEventFields.contains(key)) {
            throw ScenarioValidationError{"Unknown event field: " + key};
        }
        validateEventField(key, value, m_numericEventFields, m_eventTypeValues);
        eventFields.insert(key);
    }

    if (!inEvents) {
        throw ScenarioValidationError{"Scenario must declare an events list"};
    }

    if (eventActive) {
        ensureRequiredFields(m_requiredEventFields, eventFields, "event");
    }

    if (eventCount == 0) {
        throw ScenarioValidationError{"Scenario does not contain any events"};
    }

    ensureRequiredFields(m_requiredScenarioFields, scenarioFields, "scenario");
}

} // namespace trdp::simulation

