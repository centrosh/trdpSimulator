#pragma once

#include <string>
#include <string_view>

namespace trdp::communication {

class StackAdapter {
public:
    virtual ~StackAdapter() = default;

    virtual void openSession(const std::string &endpoint) = 0;
    virtual void closeSession() = 0;

    virtual void publishProcessData(std::string_view label) = 0;
    virtual void sendMessageData(std::string_view label) = 0;
};

} // namespace trdp::communication
