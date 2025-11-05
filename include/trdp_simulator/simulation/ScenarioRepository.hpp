#pragma once

#include "trdp_simulator/simulation/Scenario.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace trdp::device {
class DeviceProfileRepository;
}

namespace trdp::simulation {
class ScenarioSchemaValidator;
}

namespace trdp::simulation {

struct ScenarioRecord {
    std::string id;
    std::string deviceProfileId;
    std::filesystem::path storedPath;
    std::string checksum;
    std::string createdAt;
    std::string updatedAt;
};

struct RunRecord {
    std::string id;
    std::string scenarioId;
    std::filesystem::path artefactPath;
    std::string startedAt;
    std::string completedAt;
    bool success{false};
    std::string detail;
};

class ScenarioRepository {
public:
    ScenarioRepository(std::filesystem::path root, device::DeviceProfileRepository &deviceRepository,
                       ScenarioSchemaValidator &schemaValidator);

    [[nodiscard]] std::string importScenario(const std::filesystem::path &path);
    [[nodiscard]] bool exists(const std::string &id) const;
    [[nodiscard]] ScenarioRecord get(const std::string &id) const;
    [[nodiscard]] std::vector<ScenarioRecord> list() const;
    [[nodiscard]] Scenario load(const std::string &id) const;
    [[nodiscard]] Scenario loadRunScenario(const std::string &runId) const;

    void exportScenario(const std::string &id, const std::filesystem::path &destination) const;
    void recordRun(RunRecord record);
    [[nodiscard]] std::vector<RunRecord> listRuns() const;
    [[nodiscard]] std::vector<RunRecord> listRunsForScenario(const std::string &scenarioId) const;
    [[nodiscard]] RunRecord getRun(const std::string &id) const;

private:
    std::filesystem::path m_root;
    std::filesystem::path m_manifestPath;
    std::filesystem::path m_runManifestPath;
    device::DeviceProfileRepository &m_deviceRepository;
    ScenarioSchemaValidator &m_schemaValidator;
    std::unordered_map<std::string, ScenarioRecord> m_records;
    std::unordered_map<std::string, RunRecord> m_runs;

    void loadManifest();
    void persistManifest() const;
    void loadRunManifest();
    void persistRunManifest() const;
    static std::string sanitiseId(std::string candidate);
    static std::string computeChecksum(const std::filesystem::path &path);
    static std::string isoTimestamp();
};

} // namespace trdp::simulation

