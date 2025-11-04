#include "trdp_simulator/simulation/ScenarioRepository.hpp"

#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/simulation/ScenarioParser.hpp"

#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace trdp::simulation {
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

ScenarioRepository::ScenarioRepository(std::filesystem::path root, device::DeviceProfileRepository &deviceRepository)
    : m_root(std::move(root)), m_manifestPath(m_root / "manifest.db"), m_deviceRepository(deviceRepository) {
    std::filesystem::create_directories(m_root);
    loadManifest();
}

std::string ScenarioRepository::importScenario(const std::filesystem::path &path) {
    const Scenario scenario = ScenarioParser::parse(path, m_deviceRepository);

    auto candidateId = sanitiseId(scenario.id);
    if (candidateId.empty()) {
        candidateId = "scenario";
    }

    std::string uniqueId = candidateId;

    const auto storedPath = m_root / (uniqueId + ".yaml");
    if (storedPath.has_parent_path()) {
        std::filesystem::create_directories(storedPath.parent_path());
    }
    std::filesystem::copy_file(path, storedPath, std::filesystem::copy_options::overwrite_existing);

    const auto checksum = computeChecksum(storedPath);
    const auto timestamp = isoTimestamp();

    auto it = m_records.find(uniqueId);
    if (it == m_records.end()) {
        ScenarioRecord record{};
        record.id = uniqueId;
        record.deviceProfileId = scenario.deviceProfileId;
        record.storedPath = storedPath;
        record.checksum = checksum;
        record.createdAt = timestamp;
        record.updatedAt = timestamp;
        m_records.insert_or_assign(uniqueId, std::move(record));
    } else {
        it->second.deviceProfileId = scenario.deviceProfileId;
        it->second.storedPath = storedPath;
        it->second.checksum = checksum;
        if (it->second.createdAt.empty()) {
            it->second.createdAt = timestamp;
        }
        it->second.updatedAt = timestamp;
    }

    persistManifest();
    return uniqueId;
}

bool ScenarioRepository::exists(const std::string &id) const {
    return m_records.find(id) != m_records.end();
}

ScenarioRecord ScenarioRepository::get(const std::string &id) const {
    const auto it = m_records.find(id);
    if (it == m_records.end()) {
        throw std::out_of_range("Unknown scenario: " + id);
    }
    return it->second;
}

std::vector<ScenarioRecord> ScenarioRepository::list() const {
    std::vector<ScenarioRecord> records;
    records.reserve(m_records.size());
    for (const auto &[_, record] : m_records) {
        records.push_back(record);
    }
    return records;
}

Scenario ScenarioRepository::load(const std::string &id) const {
    const auto it = m_records.find(id);
    if (it == m_records.end()) {
        throw std::out_of_range("Unknown scenario: " + id);
    }
    return ScenarioParser::parse(it->second.storedPath, m_deviceRepository);
}

void ScenarioRepository::exportScenario(const std::string &id, const std::filesystem::path &destination) const {
    const auto it = m_records.find(id);
    if (it == m_records.end()) {
        throw std::out_of_range("Unknown scenario: " + id);
    }

    std::filesystem::path target = destination;
    if (std::filesystem::is_directory(destination)) {
        target /= it->second.storedPath.filename();
    } else if (destination.has_filename() && destination.extension().empty()) {
        target = destination;
        target += ".yaml";
    }

    if (target.has_parent_path()) {
        std::filesystem::create_directories(target.parent_path());
    }
    std::filesystem::copy_file(it->second.storedPath, target, std::filesystem::copy_options::overwrite_existing);
}

void ScenarioRepository::loadManifest() {
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
        if (tokens.size() < 6) {
            continue;
        }
        ScenarioRecord record{};
        record.id = tokens[0];
        record.storedPath = tokens[1];
        record.deviceProfileId = tokens[2];
        record.checksum = tokens[3];
        record.createdAt = tokens[4];
        record.updatedAt = tokens[5];
        if (!record.id.empty()) {
            m_records.insert_or_assign(record.id, std::move(record));
        }
    }
}

void ScenarioRepository::persistManifest() const {
    std::ofstream stream{m_manifestPath, std::ios::trunc};
    stream << "# id|storedPath|deviceProfileId|checksum|createdAt|updatedAt\n";
    for (const auto &[_, record] : m_records) {
        stream << record.id << '|' << record.storedPath.string() << '|' << record.deviceProfileId << '|' << record.checksum
               << '|' << record.createdAt << '|' << record.updatedAt << '\n';
    }
}

std::string ScenarioRepository::sanitiseId(std::string candidate) {
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

std::string ScenarioRepository::computeChecksum(const std::filesystem::path &path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        throw std::runtime_error("Failed to open scenario file for checksum: " + path.string());
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

std::string ScenarioRepository::isoTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

} // namespace trdp::simulation

