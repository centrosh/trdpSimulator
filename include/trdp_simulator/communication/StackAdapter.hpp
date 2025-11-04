#pragma once

#include "trdp_simulator/communication/Types.hpp"

#include <functional>
#include <string>

namespace trdp::communication {

using ProcessDataHandler = std::function<void(const ProcessDataMessage &)>;
using MessageDataHandler = std::function<void(const MessageDataMessage &)>;

class StackAdapter {
public:
    virtual ~StackAdapter() = default;

    virtual void openSession(const std::string &endpoint) = 0;
    virtual void closeSession() = 0;

    virtual void registerProcessDataHandler(ProcessDataHandler handler) = 0;
    virtual void registerMessageDataHandler(MessageDataHandler handler) = 0;

    virtual void publishProcessData(const ProcessDataMessage &message) = 0;
    virtual MessageDataAck sendMessageData(const MessageDataMessage &message) = 0;

    virtual void poll() = 0;
};

} // namespace trdp::communication
