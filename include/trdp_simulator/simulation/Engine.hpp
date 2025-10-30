#pragma once

#include "trdp_simulator/communication/Wrapper.hpp"

#include <string>
#include <vector>

namespace trdp::simulation {

struct ScenarioEvent {
    enum class Type {
        ProcessData,
        MessageData,
    };

    Type type;
    std::string label;
};

class SimulationEngine {
public:
    explicit SimulationEngine(communication::Wrapper &wrapper);

    void loadScenario(std::string name, std::vector<ScenarioEvent> events);
    void run();

    [[nodiscard]] const std::string &scenarioName() const noexcept;
    [[nodiscard]] const std::vector<ScenarioEvent> &events() const noexcept;

private:
    communication::Wrapper &m_wrapper;
    std::string m_scenarioName;
    std::vector<ScenarioEvent> m_events;
    bool m_loaded{false};
};

} // namespace trdp::simulation
