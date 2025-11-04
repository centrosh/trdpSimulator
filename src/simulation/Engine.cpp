#include "trdp_simulator/simulation/Engine.hpp"

#include "trdp_simulator/communication/Types.hpp"

#include <stdexcept>
#include <utility>

namespace trdp::simulation {

using communication::MessageDataAck;
using communication::MessageDataMessage;
using communication::MessageDataStatus;
using communication::ProcessDataMessage;

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

    try {
        for (const auto &event : m_events) {
            switch (event.type) {
            case ScenarioEvent::Type::ProcessData: {
                ProcessDataMessage message{event.label, event.comId, event.datasetId, event.payload};
                m_wrapper.publishProcessData(message);
                break;
            }
            case ScenarioEvent::Type::MessageData: {
                MessageDataMessage message{event.label, event.comId, event.datasetId, event.payload};
                const MessageDataAck ack = m_wrapper.sendMessageData(message);
                if (ack.status != MessageDataStatus::Delivered) {
                    throw std::runtime_error("Message data send failed: " + ack.detail);
                }
                break;
            }
            }
            m_wrapper.poll();
        }
        m_wrapper.close();
    } catch (...) {
        if (m_wrapper.isOpen()) {
            try {
                m_wrapper.close();
            } catch (...) {
                // Swallow close failure while propagating original exception.
            }
        }
        m_loaded = false;
        throw;
    }
    m_loaded = false;
}

const std::string &SimulationEngine::scenarioName() const noexcept { return m_scenarioName; }

const std::vector<ScenarioEvent> &SimulationEngine::events() const noexcept { return m_events; }

} // namespace trdp::simulation
