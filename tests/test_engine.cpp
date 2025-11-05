#include "trdp_simulator/communication/Types.hpp"
#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/device/XmlValidator.hpp"
#include "trdp_simulator/simulation/Engine.hpp"
#include "trdp_simulator/simulation/ScenarioRepository.hpp"
#include "trdp_simulator/simulation/ScenarioSchemaValidator.hpp"

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

using trdp::communication::DiagnosticEvent;
using trdp::communication::Wrapper;
using trdp::device::DeviceProfileRepository;
using trdp::device::XmlValidator;
using trdp::simulation::ScenarioRepository;
using trdp::simulation::ScenarioSchemaValidator;
using trdp::simulation::Scenario;
using trdp::simulation::ScenarioEvent;
using trdp::simulation::SimulationEngine;

namespace {

std::filesystem::path deviceSchemaPath() {
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

} // namespace

int main() {
    Wrapper wrapper{"integration-endpoint"};

    XmlValidator xmlValidator{deviceSchemaPath()};
    const auto deviceRoot = tempDir("engine-dev-");
    DeviceProfileRepository deviceRepository{deviceRoot, xmlValidator};
    ScenarioSchemaValidator scenarioValidator{scenarioSchemaPath()};

    const auto scenarioRoot = tempDir("engine-scenarios-");
    ScenarioRepository repository{scenarioRoot, deviceRepository, scenarioValidator};

    const auto runRoot = tempDir("engine-runs-");
    SimulationEngine engine{wrapper, runRoot, &repository};

    Scenario scenario{};
    scenario.id = "integration-smoke";
    scenario.deviceProfileId = "loopback";
    scenario.events = {
        {ScenarioEvent::Type::ProcessData, "train-ready", 1001, 1001, {0x01, 0x02}, std::chrono::milliseconds{0}},
        {ScenarioEvent::Type::MessageData, "dispatch", 2001, 2001, {0x03}, std::chrono::milliseconds{0}},
        {ScenarioEvent::Type::ProcessData, "doors-close", 1002, 1002, {0x04}, std::chrono::milliseconds{0}},
    };

    engine.loadScenario(std::move(scenario));
    engine.run();

    const auto &telemetry = wrapper.telemetry();
    // Expect open + (pd->,pd<-) + (md->,md<-) + (pd->,pd<-) + close = 8
    assert(telemetry.size() == 8);

    // Last entry should be the close event
    const std::string &lastEntry = telemetry.back();
    const std::string closeSuffix{"| close"};
    assert(lastEntry.size() >= closeSuffix.size());
    assert(lastEntry.compare(lastEntry.size() - closeSuffix.size(), closeSuffix.size(), closeSuffix) == 0);

    const auto &diagnostics = wrapper.diagnostics();
    assert(diagnostics.size() == telemetry.size());
    for (const auto &event : diagnostics) {
        assert(event.level == DiagnosticEvent::Level::Info);
    }

    // Ensure MD send recorded delivered status
    bool foundMdAck = false;
    for (const auto &entry : telemetry) {
        if (entry.find("md -> dispatch") != std::string::npos) {
            assert(entry.find("delivered") != std::string::npos);
            foundMdAck = true;
        }
    }
    assert(foundMdAck);

    const auto runs = repository.listRunsForScenario("integration-smoke");
    assert(runs.size() == 1);
    const auto &run = runs.front();
    assert(run.success);
    assert(std::filesystem::exists(run.artefactPath / "telemetry.log"));
    assert(std::filesystem::exists(run.artefactPath / "diagnostics.log"));
    assert(std::filesystem::exists(run.artefactPath / "metadata.yaml"));

    return 0;
}
