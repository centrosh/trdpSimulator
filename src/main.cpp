#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/communication/TrdpError.hpp"
#include "trdp_simulator/simulation/Engine.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using trdp::communication::DiagnosticEvent;
using trdp::communication::TrdpError;
using trdp::communication::Wrapper;
using trdp::simulation::ScenarioEvent;
using trdp::simulation::SimulationEngine;

namespace {
struct CliOptions {
    std::string scenarioName;
    std::string endpoint{"127.0.0.1"};
    std::vector<ScenarioEvent> events;
};

[[nodiscard]] ScenarioEvent::Type parseEventType(std::string_view token) {
    if (token == "pd") {
        return ScenarioEvent::Type::ProcessData;
    }
    if (token == "md") {
        return ScenarioEvent::Type::MessageData;
    }
    throw std::invalid_argument("Unknown event type: " + std::string{token});
}

CliOptions parseArgs(int argc, char **argv) {
    if (argc < 2) {
        throw std::invalid_argument("Usage: trdp-sim <scenario-name> [--endpoint <ip>] [--event <pd|md>:label]...");
    }

    CliOptions options;
    options.scenarioName = argv[1];

    for (int i = 2; i < argc; ++i) {
        const std::string arg{argv[i]};
        if (arg == "--endpoint") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--endpoint requires a value");
            }
            options.endpoint = argv[++i];
        } else if (arg == "--event") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--event requires a value");
            }
            const std::string spec{argv[++i]};
            const auto delim = spec.find(':');
            if (delim == std::string::npos || delim == spec.size() - 1) {
                throw std::invalid_argument("Event specification must be <pd|md>:label");
            }
            const auto typeToken = spec.substr(0, delim);
            const auto label = spec.substr(delim + 1);
            ScenarioEvent event{parseEventType(typeToken), label};
            options.events.push_back(event);
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    if (options.events.empty()) {
        options.events = {
            {ScenarioEvent::Type::ProcessData, "door-control"},
            {ScenarioEvent::Type::MessageData, "brake-release"},
        };
    }

    return options;
}

void printDiagnostics(const std::vector<DiagnosticEvent> &diagnostics) {
    std::cout << "Diagnostics:" << std::endl;
    for (const auto &event : diagnostics) {
        const char *level = event.level == DiagnosticEvent::Level::Info ? "INFO" : "ERROR";
        std::cout << "  [" << level << "] " << event.timestamp << " - " << event.message << std::endl;
    }
}

} // namespace

int main(int argc, char **argv) {
    CliOptions options;
    try {
        options = parseArgs(argc, argv);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    Wrapper wrapper{options.endpoint};
    SimulationEngine engine{wrapper};

    try {
        engine.loadScenario(options.scenarioName, std::move(options.events));
        engine.run();
    } catch (const TrdpError &trdp) {
        std::cerr << "TRDP failure (code " << trdp.errorCode() << ")";
        if (!trdp.context().empty()) {
            std::cerr << " context=" << trdp.context();
        }
        std::cerr << ": " << trdp.what() << std::endl;
        printDiagnostics(wrapper.diagnostics());
        return 2;
    } catch (const std::exception &ex) {
        std::cerr << "Simulation failed: " << ex.what() << std::endl;
        printDiagnostics(wrapper.diagnostics());
        return 1;
    }

    printDiagnostics(wrapper.diagnostics());

    return 0;
}
