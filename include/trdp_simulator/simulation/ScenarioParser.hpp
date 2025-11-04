#pragma once

#include "trdp_simulator/simulation/Scenario.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

namespace trdp::device {
class DeviceProfileRepository;
}

namespace trdp::simulation {

struct ScenarioValidationError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class ScenarioParser {
public:
    static Scenario parse(const std::filesystem::path &path, device::DeviceProfileRepository &repository);
};

} // namespace trdp::simulation

