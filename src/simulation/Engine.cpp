#include "trdp_simulator/simulation/Engine.hpp"

#include <stdexcept>
#include <utility>

namespace trdp::simulation {

SimulationEngine::SimulationEngine(communication::Wrapper &wrapper)
    : m_wrapper(wrapper) {}

void SimulationEngine::loadScenario(std::string name, std::vector<ScenarioEvent> events) {
    if (events.empty()) {
        throw std::invalid_argument("Scenario must contain at least one event");
    }
    m_scenarioName = std::move(name);
    m_events = std::move(events);
    m_loaded = true;
}

void SimulationEngine::run() {
    if (!m_loaded) {
        throw std::logic_error("No scenario loaded");
    }
    if (!m_wrapper.isOpen()) {
        m_wrapper.open();
    }

    for (const auto &event : m_events) {
        switch (event.type) {
        case ScenarioEvent::Type::ProcessData:
            m_wrapper.publishProcessData(event.label);
            break;
        case ScenarioEvent::Type::MessageData:
            m_wrapper.sendMessageData(event.label);
            break;
        }
    }

    m_wrapper.close();
    m_loaded = false;
}

const std::string &SimulationEngine::scenarioName() const noexcept { return m_scenarioName; }

const std::vector<ScenarioEvent> &SimulationEngine::events() const noexcept { return m_events; }

} // namespace trdp::simulation
