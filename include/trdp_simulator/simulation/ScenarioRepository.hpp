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

struct ScenarioRecord {
    std::string id;
    std::string deviceProfileId;
    std::filesystem::path storedPath;
    std::string checksum;
    std::string createdAt;
    std::string updatedAt;
};

class ScenarioRepository {
public:
    ScenarioRepository(std::filesystem::path root, device::DeviceProfileRepository &deviceRepository);

    [[nodiscard]] std::string importScenario(const std::filesystem::path &path);
    [[nodiscard]] bool exists(const std::string &id) const;
    [[nodiscard]] ScenarioRecord get(const std::string &id) const;
    [[nodiscard]] std::vector<ScenarioRecord> list() const;
    [[nodiscard]] Scenario load(const std::string &id) const;

    void exportScenario(const std::string &id, const std::filesystem::path &destination) const;

private:
    std::filesystem::path m_root;
    std::filesystem::path m_manifestPath;
    device::DeviceProfileRepository &m_deviceRepository;
    std::unordered_map<std::string, ScenarioRecord> m_records;

    void loadManifest();
    void persistManifest() const;
    static std::string sanitiseId(std::string candidate);
    static std::string computeChecksum(const std::filesystem::path &path);
    static std::string isoTimestamp();
};

} // namespace trdp::simulation

