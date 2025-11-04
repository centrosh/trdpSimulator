#pragma once

#include "trdp_simulator/communication/Wrapper.hpp"
#include "trdp_simulator/simulation/Scenario.hpp"

#include <cstdint>
#include <string>

namespace trdp::simulation {

class SimulationEngine {
public:
    explicit SimulationEngine(communication::Wrapper &wrapper);

    void loadScenario(Scenario scenario);
    void run();

    [[nodiscard]] const Scenario &scenario() const noexcept;

private:
    communication::Wrapper &m_wrapper;
    Scenario m_scenario;
    bool m_loaded{false};
};

} // namespace trdp::simulation

