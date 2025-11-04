#include "trdp_simulator/simulation/Engine.hpp"

#include "trdp_simulator/communication/Types.hpp"

#include <chrono>
#include <stdexcept>
#include <thread>

namespace trdp::simulation {

using communication::MessageDataAck;
using communication::MessageDataMessage;
using communication::MessageDataStatus;
using communication::ProcessDataMessage;

SimulationEngine::SimulationEngine(communication::Wrapper &wrapper)
    : m_wrapper(wrapper) {}

void SimulationEngine::loadScenario(Scenario scenario) {
    if (scenario.events.empty()) {
        throw std::invalid_argument("Scenario must contain at least one event");
    }
    if (scenario.deviceProfileId.empty()) {
        throw std::invalid_argument("Scenario requires a device profile");
    }
    m_scenario = std::move(scenario);
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
        for (const auto &event : m_scenario.events) {
            if (event.delay.count() > 0) {
                std::this_thread::sleep_for(event.delay);
            }
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
            }
        }
        m_loaded = false;
        throw;
    }
    m_loaded = false;
}

const Scenario &SimulationEngine::scenario() const noexcept {
    return m_scenario;
}

} // namespace trdp::simulation

