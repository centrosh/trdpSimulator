#include "trdp_simulator/simulation/Engine.hpp"

#include "trdp_simulator/communication/Types.hpp"
#include "trdp_simulator/simulation/ScenarioRepository.hpp"
#include "trdp_simulator/simulation/ScenarioYaml.hpp"

#include <chrono>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <thread>

namespace trdp::simulation {

using communication::MessageDataAck;
using communication::MessageDataMessage;
using communication::MessageDataStatus;
using communication::ProcessDataMessage;

namespace {

[[nodiscard]] std::string isoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

[[nodiscard]] std::string safeTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &time);
#else
    gmtime_r(&time, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%dT%H%M%SZ");
    return oss.str();
}

[[nodiscard]] std::string sanitiseId(const std::string &candidate) {
    std::string result;
    result.reserve(candidate.size());
    for (char ch : candidate) {
        if (std::isalnum(static_cast<unsigned char>(ch)) || ch == '-' || ch == '_') {
            result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
        } else if (std::isspace(static_cast<unsigned char>(ch))) {
            result.push_back('-');
        }
    }
    return result;
}

[[nodiscard]] std::string payloadToString(const std::vector<std::uint8_t> &payload) {
    if (payload.empty()) {
        return "";
    }
    std::ostringstream oss;
    oss << "0x";
    oss << std::hex << std::setfill('0');
    for (std::uint8_t byte : payload) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

void writeScenarioFile(const std::filesystem::path &path, const Scenario &scenario) {
    std::ofstream stream{path, std::ios::trunc};
    stream << "scenario: " << scenario.id << '\n';
    stream << "device: " << scenario.deviceProfileId << '\n';
    stream << "events:\n";
    for (const auto &event : scenario.events) {
        stream << "  - type: " << (event.type == ScenarioEvent::Type::ProcessData ? "pd" : "md") << '\n';
        stream << "    label: " << event.label << '\n';
        if (event.comId != 0) {
            stream << "    com_id: " << event.comId << '\n';
        }
        if (event.datasetId != 0) {
            stream << "    dataset_id: " << event.datasetId << '\n';
        }
        const auto payloadStr = payloadToString(event.payload);
        if (!payloadStr.empty()) {
            stream << "    payload: " << payloadStr << '\n';
        }
        if (event.delay.count() > 0) {
            stream << "    delay_ms: " << event.delay.count() << '\n';
        }
    }
}

void writeTelemetryFile(const std::filesystem::path &path, const std::vector<std::string> &entries) {
    std::ofstream stream{path, std::ios::trunc};
    for (const auto &entry : entries) {
        stream << entry << '\n';
    }
}

void writeDiagnosticsFile(const std::filesystem::path &path, const std::vector<communication::DiagnosticEvent> &events) {
    std::ofstream stream{path, std::ios::trunc};
    for (const auto &event : events) {
        const char *level = event.level == communication::DiagnosticEvent::Level::Info ? "INFO" : "ERROR";
        stream << level << '|' << event.timestamp << '|' << event.message << '\n';
    }
}

[[nodiscard]] std::string sanitiseDetail(std::string detail) {
    for (char &ch : detail) {
        if (ch == '\n' || ch == '\r') {
            ch = ' ';
        }
    }
    return detail;
}

void writeMetadataFile(const std::filesystem::path &path, const std::string &runId, const Scenario &scenario,
                       const std::string &startedAt, const std::string &completedAt, bool success,
                       std::string_view detail) {
    std::ofstream stream{path, std::ios::trunc};
    stream << "run_id: " << runId << '\n';
    stream << "scenario_id: " << scenario.id << '\n';
    stream << "device_profile: " << scenario.deviceProfileId << '\n';
    stream << "started_at: " << startedAt << '\n';
    stream << "completed_at: " << completedAt << '\n';
    stream << "success: " << (success ? "true" : "false") << '\n';
    if (!detail.empty()) {
        stream << "detail: " << sanitiseDetail(std::string{detail}) << '\n';
    }
}

struct RunContext {
    std::string id;
    std::string startedAt;
    std::filesystem::path directory;
    std::ofstream eventLog;
};

RunContext prepareRunContext(const Scenario &scenario, const std::filesystem::path &root) {
    RunContext context;
    context.startedAt = isoTimestamp();
    auto baseId = sanitiseId(scenario.id.empty() ? "scenario" : scenario.id);
    if (baseId.empty()) {
        baseId = "scenario";
    }
    context.id = baseId + '-' + safeTimestamp();
    context.directory = root / context.id;
    std::filesystem::create_directories(context.directory);
    context.eventLog.open(context.directory / "events.log", std::ios::out | std::ios::trunc);
    if (!context.eventLog) {
        throw std::runtime_error("Failed to open run event log: " + (context.directory / "events.log").string());
    }
    writeScenarioFile(context.directory / "scenario.yaml", scenario);
    return context;
}

} // namespace

SimulationEngine::SimulationEngine(communication::Wrapper &wrapper, std::filesystem::path artefactRoot,
                                   ScenarioRepository *repository)
    : m_wrapper(wrapper), m_artefactRoot(std::move(artefactRoot)), m_repository(repository) {
    if (!m_artefactRoot.empty()) {
        std::filesystem::create_directories(m_artefactRoot);
    }
}

void SimulationEngine::loadScenario(Scenario scenario) {
    if (scenario.events.empty()) {
        throw std::invalid_argument("Scenario must contain at least one event");
    }
    if (scenario.deviceProfileId.empty()) {
        throw std::invalid_argument("Scenario requires a device profile");
    }
    m_scenario = std::move(scenario);
    m_loaded = true;
}

void SimulationEngine::run() {
    if (!m_loaded) {
        throw std::logic_error("No scenario loaded");
    }
    if (!m_wrapper.isOpen()) {
        m_wrapper.open();
    }

    std::optional<RunContext> runContext;
    if (!m_artefactRoot.empty()) {
        runContext = prepareRunContext(m_scenario, m_artefactRoot);
    }

    const auto finaliseRun = [&](bool success, std::string_view detail) {
        if (!runContext) {
            return;
        }
        if (runContext->eventLog.is_open()) {
            runContext->eventLog.flush();
            runContext->eventLog.close();
        }
        const auto completedAt = isoTimestamp();
        writeTelemetryFile(runContext->directory / "telemetry.log", m_wrapper.telemetry());
        writeDiagnosticsFile(runContext->directory / "diagnostics.log", m_wrapper.diagnostics());
        writeMetadataFile(runContext->directory / "metadata.yaml", runContext->id, m_scenario, runContext->startedAt,
                          completedAt, success, detail);
        if (m_repository != nullptr) {
            RunRecord record{};
            record.id = runContext->id;
            record.scenarioId = m_scenario.id;
            record.artefactPath = runContext->directory;
            record.startedAt = runContext->startedAt;
            record.completedAt = completedAt;
            record.success = success;
            record.detail = std::string(detail);
            m_repository->recordRun(std::move(record));
        }
    };

    try {
        for (const auto &event : m_scenario.events) {
            if (event.delay.count() > 0) {
                std::this_thread::sleep_for(event.delay);
            }
            if (runContext && runContext->eventLog.is_open()) {
                runContext->eventLog << isoTimestamp() << " | " << scenario_yaml::describeEvent(event) << '\n';
            }
            switch (event.type) {
            case ScenarioEvent::Type::ProcessData: {
                ProcessDataMessage message{event.label, event.comId, event.datasetId, event.payload};
                m_wrapper.publishProcessData(message);
                break;
            }
            case ScenarioEvent::Type::MessageData: {
                MessageDataMessage message{event.label, event.comId, event.datasetId, event.payload};
                const MessageDataAck ack = m_wrapper.sendMessageData(message);
                if (ack.status != MessageDataStatus::Delivered) {
                    throw std::runtime_error("Message data send failed: " + ack.detail);
                }
                break;
            }
            }
            m_wrapper.poll();
        }
        m_wrapper.close();
        finaliseRun(true, {});
    } catch (...) {
        if (m_wrapper.isOpen()) {
            try {
                m_wrapper.close();
            } catch (...) {
            }
        }
        try {
            throw;
        } catch (const std::exception &ex) {
            finaliseRun(false, ex.what());
            m_loaded = false;
            throw;
        } catch (...) {
            finaliseRun(false, "unknown failure");
            m_loaded = false;
            throw;
        }
    }
    m_loaded = false;
}

const Scenario &SimulationEngine::scenario() const noexcept {
    return m_scenario;
}

} // namespace trdp::simulation

