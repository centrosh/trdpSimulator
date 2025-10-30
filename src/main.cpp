#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/simulation/Engine.hpp"

#include <exception>
#include <iostream>
#include <string>
#include <vector>

using trdp::communication::Wrapper;
using trdp::simulation::ScenarioEvent;
using trdp::simulation::SimulationEngine;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: trdp-sim <scenario-name>" << std::endl;
        return 1;
    }

    const std::string scenarioName = argv[1];

    Wrapper wrapper{"127.0.0.1"};
    SimulationEngine engine{wrapper};

    std::vector<ScenarioEvent> events{
        {ScenarioEvent::Type::ProcessData, "door-control"},
        {ScenarioEvent::Type::MessageData, "brake-release"},
    };

    try {
        engine.loadScenario(scenarioName, std::move(events));
        engine.run();
    } catch (const std::exception &ex) {
        std::cerr << "Simulation failed: " << ex.what() << std::endl;
        return 1;
    }

    const auto &telemetry = wrapper.telemetry();
    std::cout << "Simulation completed. Telemetry events:" << std::endl;
    for (const auto &entry : telemetry) {
        std::cout << "  - " << entry << std::endl;
    }

    return 0;
}
