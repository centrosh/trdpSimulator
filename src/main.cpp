#include "trdp_simulator/communication/Types.hpp"
#include "trdp_simulator/communication/TrdpError.hpp"
#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/communication/TrdpError.hpp"
#include "trdp_simulator/simulation/Engine.hpp"

#include <exception>
#include <iostream>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

using trdp::communication::DiagnosticEvent;
using trdp::communication::MessageDataMessage;
using trdp::communication::ProcessDataMessage;
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

[[nodiscard]] std::vector<std::string> splitTokens(std::string_view input) {
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= input.size()) {
        const auto pos = input.find(':', start);
        if (pos == std::string_view::npos) {
            tokens.emplace_back(input.substr(start));
            break;
        }
        tokens.emplace_back(input.substr(start, pos - start));
        start = pos + 1;
        if (start == input.size()) {
            tokens.emplace_back("");
            break;
        }
    }
    return tokens;
}

[[nodiscard]] std::vector<std::uint8_t> parsePayloadToken(std::string_view token) {
    std::vector<std::uint8_t> payload;
    if (token.empty()) {
        return payload;
    }
    if (token.size() > 2 && (token.rfind("0x", 0) == 0 || token.rfind("0X", 0) == 0)) {
        if ((token.size() - 2) % 2 != 0) {
            throw std::invalid_argument("Hex payload must contain an even number of characters");
        }
        for (std::size_t i = 2; i < token.size(); i += 2) {
            const auto byteStr = token.substr(i, 2);
            const unsigned value = std::stoul(std::string{byteStr}, nullptr, 16);
            if (value > 0xFF) {
                throw std::invalid_argument("Hex payload byte out of range: " + std::string{byteStr});
            }
            payload.push_back(static_cast<std::uint8_t>(value));
        }
        return payload;
    }
    payload.assign(token.begin(), token.end());
    return payload;
}

CliOptions parseArgs(int argc, char **argv) {
    if (argc < 2) {
        throw std::invalid_argument("Usage: trdp-sim <scenario-name> [--endpoint <ip>] [--event <pd|md>:label[:comId][:dataset][:payload]]...");
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
            const auto tokens = splitTokens(spec);
            if (tokens.size() < 2) {
                throw std::invalid_argument("Event specification must be <pd|md>:label[:comId][:dataset][:payload]");
            }
            ScenarioEvent event{};
            event.type = parseEventType(tokens[0]);
            event.label = tokens[1];
            if (tokens.size() >= 3 && !tokens[2].empty()) {
                event.comId = static_cast<std::uint32_t>(std::stoul(tokens[2]));
            }
            if (tokens.size() >= 4 && !tokens[3].empty()) {
                event.datasetId = static_cast<std::uint32_t>(std::stoul(tokens[3]));
            }
            if (tokens.size() >= 5 && !tokens[4].empty()) {
                event.payload = parsePayloadToken(tokens[4]);
            }
            options.events.push_back(std::move(event));
        } else {
            throw std::invalid_argument("Unknown argument: " + arg);
        }
    }

    if (options.events.empty()) {
        options.events = {
            {ScenarioEvent::Type::ProcessData, "door-control", 1001, 1001, {0x01, 0x02}},
            {ScenarioEvent::Type::MessageData, "brake-release", 2001, 2001, {0x7B}},
            {ScenarioEvent::Type::ProcessData, "doors-closed", 1002, 1002, {0x05}},
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

void registerLoopbackLogging(Wrapper &wrapper) {
    wrapper.registerProcessDataHandler([](const ProcessDataMessage &message) {
        std::cout << "PD received: " << message.label << " (bytes=" << message.payload.size() << ")" << std::endl;
    });
    wrapper.registerMessageDataHandler([](const MessageDataMessage &message) {
        std::cout << "MD received: " << message.label << " (bytes=" << message.payload.size() << ")" << std::endl;
    });
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
    registerLoopbackLogging(wrapper);
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
