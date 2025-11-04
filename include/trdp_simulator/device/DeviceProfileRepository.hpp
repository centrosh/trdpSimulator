#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace trdp::device {

struct DeviceProfileRecord {
    std::string id;
    std::filesystem::path storedPath;
    std::filesystem::path sourcePath;
    std::string checksum;
    std::string validatedAt;
};

class XmlValidator;

class DeviceProfileRepository {
public:
    DeviceProfileRepository(std::filesystem::path root, XmlValidator &validator);

    [[nodiscard]] std::string registerProfile(const std::filesystem::path &xmlPath);
    [[nodiscard]] bool exists(const std::string &id) const;
    [[nodiscard]] DeviceProfileRecord get(const std::string &id) const;
    [[nodiscard]] std::vector<DeviceProfileRecord> list() const;

    void markValidated(const std::string &id, std::string timestamp);

private:
    std::filesystem::path m_root;
    std::filesystem::path m_manifestPath;
    XmlValidator &m_validator;

    std::unordered_map<std::string, DeviceProfileRecord> m_records;

    void loadManifest();
    void persistManifest() const;
    static std::string sanitiseId(std::string candidate);
    static std::string computeChecksum(const std::filesystem::path &path);
    static std::string isoTimestamp();
};

} // namespace trdp::device

