#include "trdp_simulator/communication/Types.hpp"
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
        {ScenarioEvent::Type::ProcessData, "train-ready", 1001, 1001, {0x01, 0x02}},
        {ScenarioEvent::Type::MessageData, "dispatch", 2001, 2001, {0x03}},
        {ScenarioEvent::Type::ProcessData, "doors-close", 1002, 1002, {0x04}},
    };

    engine.loadScenario("integration-smoke", events);
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

    return 0;
}
