#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/device/XmlValidator.hpp"
#include "trdp_simulator/simulation/ScenarioLoader.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using trdp::device::DeviceProfileRepository;
using trdp::device::XmlValidator;
using trdp::simulation::Scenario;
using trdp::simulation::ScenarioLoader;

namespace {

std::filesystem::path schemaPath() {
    const auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "resources/trdp/trdp-config.xsd";
}

std::filesystem::path tempDir(const std::string &name) {
    auto dir = std::filesystem::temp_directory_path() / std::filesystem::path{name + std::to_string(std::rand())};
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

int main() {
    XmlValidator validator{schemaPath()};
    const auto repoRoot = tempDir("scenario-repo-");
    DeviceProfileRepository repository{repoRoot, validator};

    const auto deviceId = repository.registerProfile(schemaPath().parent_path() / "device1.xml");

    const auto scenarioRoot = tempDir("scenario-files-");
    ScenarioLoader loader{repository, scenarioRoot};

    const auto scenarioPath = scenarioRoot / "door.yaml";
    std::ofstream scenarioFile{scenarioPath};
    scenarioFile << "scenario: door\n";
    scenarioFile << "device: " << deviceId << "\n";
    scenarioFile << "events:\n";
    scenarioFile << "  - type: pd\n";
    scenarioFile << "    label: command\n";
    scenarioFile << "    com_id: 1001\n";
    scenarioFile << "    dataset_id: 1001\n";
    scenarioFile << "    payload: 0x0102\n";
    scenarioFile.close();

    const Scenario scenario = loader.load("door");
    assert(scenario.id == "door");
    assert(scenario.deviceProfileId == deviceId);
    assert(scenario.events.size() == 1);
    assert(scenario.events.front().label == "command");
    assert(scenario.events.front().payload.size() == 2);

    const auto adhocPath = repoRoot / "adhoc.yaml";
    std::ofstream adhoc{adhocPath};
    adhoc << "scenario: adhoc\n";
    adhoc << "device: unknown\n";
    adhoc << "events:\n";
    adhoc << "  - type: md\n";
    adhoc << "    label: test\n";
    adhoc.close();

    bool threw = false;
    try {
        (void)loader.loadFromFile(adhocPath);
    } catch (const std::exception &) {
        threw = true;
    }
    assert(threw);

    return 0;
}

