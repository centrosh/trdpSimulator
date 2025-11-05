#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/device/XmlValidator.hpp"
#include "trdp_simulator/simulation/ScenarioRepository.hpp"
#include "trdp_simulator/simulation/ScenarioSchemaValidator.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using trdp::device::DeviceProfileRepository;
using trdp::device::XmlValidator;
using trdp::simulation::Scenario;
using trdp::simulation::ScenarioRepository;
using trdp::simulation::ScenarioSchemaValidator;
using trdp::simulation::RunRecord;

namespace {

std::filesystem::path schemaPath() {
    const auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "resources/trdp/trdp-config.xsd";
}

std::filesystem::path scenarioSchemaPath() {
    const auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "resources/scenarios/scenario.schema.yaml";
}

std::filesystem::path tempDir(const std::string &name) {
    auto dir = std::filesystem::temp_directory_path() / std::filesystem::path{name + std::to_string(std::rand())};
    std::filesystem::create_directories(dir);
    return dir;
}

std::filesystem::path writeScenario(const std::filesystem::path &path, const std::string &deviceId, std::string payload) {
    std::ofstream scenarioFile{path};
    scenarioFile << "scenario: " << path.stem().string() << "\n";
    scenarioFile << "device: " << deviceId << "\n";
    scenarioFile << "events:\n";
    scenarioFile << "  - type: pd\n";
    scenarioFile << "    label: command\n";
    scenarioFile << "    com_id: 1001\n";
    scenarioFile << "    dataset_id: 1001\n";
    scenarioFile << "    payload: " << payload << "\n";
    scenarioFile.close();
    return path;
}

} // namespace

int main() {
    XmlValidator validator{schemaPath()};
    const auto deviceRoot = tempDir("scenario-device-");
    DeviceProfileRepository deviceRepository{deviceRoot, validator};
    const auto deviceId = deviceRepository.registerProfile(schemaPath().parent_path() / "device1.xml");

    const auto scenarioRoot = tempDir("scenario-store-");
    ScenarioSchemaValidator scenarioValidator{scenarioSchemaPath()};
    ScenarioRepository repository{scenarioRoot, deviceRepository, scenarioValidator};

    const auto tempScenario = tempDir("scenario-src-") / "door.yaml";
    writeScenario(tempScenario, deviceId, "0x0102");

    const auto storedId = repository.importScenario(tempScenario);
    assert(storedId == "door");
    assert(repository.exists(storedId));

    const Scenario loaded = repository.load(storedId);
    assert(loaded.id == "door");
    assert(loaded.deviceProfileId == deviceId);
    assert(loaded.events.size() == 1);
    assert(loaded.events.front().payload.size() == 2);

    const auto updatedScenario = tempDir("scenario-src-") / "door.yaml";
    writeScenario(updatedScenario, deviceId, "0x0A0B");
    const auto updatedId = repository.importScenario(updatedScenario);
    assert(updatedId == storedId);

    const auto exportPath = tempDir("scenario-export-") / "door_copy.yaml";
    repository.exportScenario(storedId, exportPath);
    assert(std::filesystem::exists(exportPath));
    const auto exportDevice = exportPath.parent_path() / "devices" / (deviceId + ".xml");
    assert(std::filesystem::exists(exportDevice));

    const auto exportDir = tempDir("scenario-export-dir-");
    repository.exportScenario(storedId, exportDir);
    const auto dirScenarioPath = exportDir / "door.yaml";
    assert(std::filesystem::exists(dirScenarioPath));
    const auto dirDevice = exportDir / "devices" / (deviceId + ".xml");
    assert(std::filesystem::exists(dirDevice));

    const auto records = repository.list();
    assert(records.size() == 1);
    assert(records.front().id == storedId);

    bool threw = false;
    try {
        (void)repository.load("missing");
    } catch (const std::exception &) {
        threw = true;
    }
    assert(threw);

    RunRecord runRecord{};
    runRecord.id = "door-run";
    runRecord.scenarioId = storedId;
    runRecord.artefactPath = tempDir("scenario-run-");
    runRecord.startedAt = "2024-01-01T00:00:00Z";
    runRecord.completedAt = "2024-01-01T00:01:00Z";
    runRecord.success = true;
    repository.recordRun(runRecord);

    const auto runs = repository.listRunsForScenario(storedId);
    assert(!runs.empty());
    assert(runs.front().id == runRecord.id);

    return 0;
}

