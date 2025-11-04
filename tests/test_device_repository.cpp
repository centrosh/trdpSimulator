#include "trdp_simulator/device/DeviceProfileRepository.hpp"
#include "trdp_simulator/device/XmlValidator.hpp"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using trdp::device::DeviceProfileRepository;
using trdp::device::DeviceProfileRecord;
using trdp::device::XmlValidator;

namespace {

std::filesystem::path schemaPath() {
    const auto repoRoot = std::filesystem::path(__FILE__).parent_path().parent_path();
    return repoRoot / "resources/trdp/trdp-config.xsd";
}

std::filesystem::path uniqueTempDir() {
    auto dir = std::filesystem::temp_directory_path() / std::filesystem::path{"trdp-device-test" + std::to_string(std::rand())};
    std::filesystem::create_directories(dir);
    return dir;
}

} // namespace

int main() {
    XmlValidator validator{schemaPath()};
    const auto root = uniqueTempDir();
    DeviceProfileRepository repository{root, validator};

    const auto validXml = schemaPath().parent_path() / "device1.xml";
    const auto profileId = repository.registerProfile(validXml);
    assert(!profileId.empty());
    assert(repository.exists(profileId));

    const DeviceProfileRecord record = repository.get(profileId);
    assert(record.id == profileId);
    assert(std::filesystem::exists(record.storedPath));
    assert(!record.checksum.empty());
    assert(!record.validatedAt.empty());

    const auto sameId = repository.registerProfile(validXml);
    assert(sameId == profileId);

    // Invalid XML should throw and not create a profile
    const auto invalidPath = root / "invalid.xml";
    std::ofstream invalidFile{invalidPath};
    invalidFile << "<device></invalid>";
    invalidFile.close();

    bool threw = false;
    try {
        (void)repository.registerProfile(invalidPath);
    } catch (const std::exception &) {
        threw = true;
    }
    assert(threw);
    assert(!repository.exists("invalid"));

    return 0;
}

