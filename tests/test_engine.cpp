#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/simulation/Engine.hpp"

#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

using trdp::communication::DiagnosticEvent;

using trdp::communication::Wrapper;
using trdp::simulation::ScenarioEvent;
using trdp::simulation::SimulationEngine;

int main() {
    Wrapper wrapper{"integration-endpoint"};
    SimulationEngine engine{wrapper};

    std::vector<ScenarioEvent> events{
        {ScenarioEvent::Type::ProcessData, "train-ready"},
        {ScenarioEvent::Type::MessageData, "dispatch"},
        {ScenarioEvent::Type::ProcessData, "doors-close"},
    };

    engine.loadScenario("integration-smoke", events);
    engine.run();

    const auto &telemetry = wrapper.telemetry();
    // Expect open + 3 events + close
    assert(telemetry.size() == 5);

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

    return 0;
}
