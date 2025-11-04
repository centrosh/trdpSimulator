#include "trdp_simulator/device/DeviceProfileRepository.hpp"

#include "trdp_simulator/device/XmlValidator.hpp"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

namespace trdp::device {

namespace {

[[nodiscard]] std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

[[nodiscard]] std::vector<std::string> split(const std::string &line, char delim) {
    std::vector<std::string> tokens;
    std::stringstream ss{line};
    std::string token;
    while (std::getline(ss, token, delim)) {
        tokens.emplace_back(std::move(token));
    }
    return tokens;
}

} // namespace

DeviceProfileRepository::DeviceProfileRepository(std::filesystem::path root, XmlValidator &validator)
    : m_root(std::move(root)), m_manifestPath(m_root / "manifest.db"), m_validator(validator) {
    std::filesystem::create_directories(m_root);
    loadManifest();
}

std::string DeviceProfileRepository::registerProfile(const std::filesystem::path &xmlPath) {
    if (!std::filesystem::exists(xmlPath)) {
        throw std::invalid_argument("XML file does not exist: " + xmlPath.string());
    }

    const auto checksum = computeChecksum(xmlPath);
    for (const auto &[id, record] : m_records) {
        if (record.checksum == checksum) {
            return id;
        }
    }

    auto candidateId = sanitiseId(xmlPath.stem().string());
    if (candidateId.empty()) {
        candidateId = "device";
    }

    std::unordered_set<std::string> ids;
    for (const auto &[id, _] : m_records) {
        ids.insert(id);
    }

    std::string uniqueId = candidateId;
    int suffix = 1;
    while (ids.contains(uniqueId)) {
        uniqueId = candidateId + "-" + std::to_string(++suffix);
    }

    const auto storedPath = m_root / (uniqueId + ".xml");
    std::filesystem::copy_file(xmlPath, storedPath, std::filesystem::copy_options::overwrite_existing);

    const auto result = m_validator.validate(storedPath);
    if (!result.success) {
        std::filesystem::remove(storedPath);
        throw std::runtime_error("XML validation failed: " + result.message);
    }

    DeviceProfileRecord record{};
    record.id = uniqueId;
    record.storedPath = storedPath;
    record.sourcePath = std::filesystem::absolute(xmlPath);
    record.checksum = checksum;
    record.validatedAt = isoTimestamp();

    m_records.insert_or_assign(uniqueId, record);
    persistManifest();
    return uniqueId;
}

bool DeviceProfileRepository::exists(const std::string &id) const {
    return m_records.find(id) != m_records.end();
}

DeviceProfileRecord DeviceProfileRepository::get(const std::string &id) const {
    const auto it = m_records.find(id);
    if (it == m_records.end()) {
        throw std::out_of_range("Unknown device profile: " + id);
    }
    return it->second;
}

std::vector<DeviceProfileRecord> DeviceProfileRepository::list() const {
    std::vector<DeviceProfileRecord> records;
    records.reserve(m_records.size());
    for (const auto &[_, record] : m_records) {
        records.push_back(record);
    }
    return records;
}

void DeviceProfileRepository::markValidated(const std::string &id, std::string timestamp) {
    auto it = m_records.find(id);
    if (it == m_records.end()) {
        throw std::out_of_range("Unknown device profile: " + id);
    }
    it->second.validatedAt = std::move(timestamp);
    persistManifest();
}

void DeviceProfileRepository::loadManifest() {
    m_records.clear();
    if (!std::filesystem::exists(m_manifestPath)) {
        return;
    }

    std::ifstream stream{m_manifestPath};
    std::string line;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty() || line.starts_with('#')) {
            continue;
        }
        const auto tokens = split(line, '|');
        if (tokens.size() < 5) {
            continue;
        }
        DeviceProfileRecord record{};
        record.id = tokens[0];
        record.storedPath = tokens[1];
        record.sourcePath = tokens[2];
        record.checksum = tokens[3];
        record.validatedAt = tokens[4];
        if (!record.id.empty()) {
            m_records.insert_or_assign(record.id, std::move(record));
        }
    }
}

void DeviceProfileRepository::persistManifest() const {
    std::ofstream stream{m_manifestPath, std::ios::trunc};
    stream << "# id|storedPath|sourcePath|checksum|validatedAt\n";
    for (const auto &[_, record] : m_records) {
        stream << record.id << '|' << record.storedPath.string() << '|' << record.sourcePath.string()
               << '|' << record.checksum << '|' << record.validatedAt << '\n';
    }
}

std::string DeviceProfileRepository::sanitiseId(std::string candidate) {
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

std::string DeviceProfileRepository::computeChecksum(const std::filesystem::path &path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        throw std::runtime_error("Failed to open XML file for checksum: " + path.string());
    }
    constexpr std::uint64_t fnvOffset = 1469598103934665603ull;
    constexpr std::uint64_t fnvPrime = 1099511628211ull;
    std::uint64_t hash = fnvOffset;
    char buffer[4096];
    while (stream.read(buffer, sizeof(buffer)) || stream.gcount() > 0) {
        const std::streamsize count = stream.gcount();
        for (std::streamsize i = 0; i < count; ++i) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= fnvPrime;
        }
    }
    std::ostringstream oss;
    oss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return oss.str();
}

std::string DeviceProfileRepository::isoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace trdp::device

