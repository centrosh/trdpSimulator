#pragma once

#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/simulation/Scenario.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace trdp::simulation {

class ScenarioRepository;

class SimulationEngine {
public:
    explicit SimulationEngine(communication::Wrapper &wrapper, std::filesystem::path artefactRoot = {},
                              ScenarioRepository *repository = nullptr);

    void loadScenario(Scenario scenario);
    void run();

    [[nodiscard]] const Scenario &scenario() const noexcept;

private:
    communication::Wrapper &m_wrapper;
    std::filesystem::path m_artefactRoot;
    ScenarioRepository *m_repository{nullptr};
    Scenario m_scenario;
    bool m_loaded{false};
};

} // namespace trdp::simulation

