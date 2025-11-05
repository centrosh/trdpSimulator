#include "trdp_simulator/simulation/ScenarioSchemaValidator.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using trdp::simulation::ScenarioSchemaValidator;
using trdp::simulation::ScenarioValidationError;

namespace {

std::filesystem::path scenarioSchemaPath() {
    const auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "resources/scenarios/scenario.schema.yaml";
}

std::filesystem::path tempDir(const std::string &name) {
    auto dir = std::filesystem::temp_directory_path() / std::filesystem::path{name + std::to_string(std::rand())};
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path writeValidScenario(const std::filesystem::path &path) {
    std::ofstream scenarioFile{path};
    scenarioFile << "scenario: loopback\n";
    scenarioFile << "device: device1\n";
    scenarioFile << "events:\n";
    scenarioFile << "  - type: pd\n";
    scenarioFile << "    label: start\n";
    scenarioFile << "    com_id: 1001\n";
    scenarioFile << "    dataset_id: 1001\n";
    scenarioFile << "    payload: 0x01\n";
    scenarioFile.close();
    return path;
}

std::filesystem::path writeInvalidScenario(const std::filesystem::path &path) {
    std::ofstream scenarioFile{path};
    scenarioFile << "scenario: invalid\n";
    scenarioFile << "events:\n";
    scenarioFile << "  - type: md\n";
    scenarioFile << "    label: ack\n";
    scenarioFile.close();
    return path;
}

} // namespace

int main() {
    ScenarioSchemaValidator validator{scenarioSchemaPath()};

    const auto workingDir = tempDir("schema-validator-");
    const auto validScenario = writeValidScenario(workingDir / "valid.yaml");
    validator.validate(validScenario);

    const auto invalidScenario = writeInvalidScenario(workingDir / "invalid.yaml");
    bool threw = false;
    try {
        validator.validate(invalidScenario);
    } catch (const ScenarioValidationError &) {
        threw = true;
    }
    assert(threw);

    return 0;
}
