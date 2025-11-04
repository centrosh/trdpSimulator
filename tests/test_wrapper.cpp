#include "trdp_simulator/communication/TrdpError.hpp"
#include "trdp_simulator/communication/Types.hpp"
#include "trdp_simulator/communication/Wrapper.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using trdp::communication::DiagnosticEvent;
using trdp::communication::MessageDataAck;
using trdp::communication::MessageDataMessage;
using trdp::communication::MessageDataStatus;
using trdp::communication::ProcessDataMessage;
using trdp::communication::StackAdapter;
using trdp::communication::TrdpError;
using trdp::communication::Wrapper;

namespace {
class RecordingAdapter final : public StackAdapter {
public:
    void openSession(const std::string &endpoint) override {
        if (failOnOpen) {
            throw TrdpError("forced open failure", 42, endpoint);
        }
        if (openCalled) {
            throw TrdpError("duplicate open", 43, endpoint);
        }
        openCalled = true;
        lastEndpoint = endpoint;
    }

    void closeSession() override {
        if (!openCalled) {
            throw TrdpError("close without open", 44, lastEndpoint);
        }
        openCalled = false;
        if (failOnClose) {
            throw TrdpError("forced close failure", 45, lastEndpoint);
        }
    }

    void registerProcessDataHandler(trdp::communication::ProcessDataHandler handler) override { processHandler = std::move(handler); }

    void registerMessageDataHandler(trdp::communication::MessageDataHandler handler) override { messageHandler = std::move(handler); }

    void publishProcessData(const ProcessDataMessage &message) override {
        if (!openCalled) {
            throw TrdpError("pd without open", 46, message.label);
        }
        lastPd = message;
        if (failOnPd) {
            throw TrdpError("forced pd failure", 47, message.label);
        }
        if (processHandler) {
            processHandler(message);
        }
    }

    MessageDataAck sendMessageData(const MessageDataMessage &message) override {
        if (!openCalled) {
            throw TrdpError("md without open", 48, message.label);
        }
        lastMd = message;
        if (failOnMd) {
            throw TrdpError("forced md failure", 49, message.label);
        }
        if (messageHandler) {
            messageHandler(message);
        }
        return ackToReturn;
    }

    void poll() override {
        if (failOnPoll) {
            throw TrdpError("poll failure", 50, lastEndpoint);
        }
        if (!pendingProcessData.empty() && processHandler) {
            processHandler(pendingProcessData.back());
            pendingProcessData.pop_back();
        }
        if (!pendingMessageData.empty() && messageHandler) {
            messageHandler(pendingMessageData.back());
            pendingMessageData.pop_back();
        }
    }

    void queueProcessData(ProcessDataMessage message) { pendingProcessData.push_back(std::move(message)); }
    void queueMessageData(MessageDataMessage message) { pendingMessageData.push_back(std::move(message)); }

    bool openCalled{false};
    bool failOnOpen{false};
    bool failOnClose{false};
    bool failOnPd{false};
    bool failOnMd{false};
    bool failOnPoll{false};
    std::string lastEndpoint;
    ProcessDataMessage lastPd;
    MessageDataMessage lastMd;
    MessageDataAck ackToReturn{MessageDataStatus::Delivered, "ack"};
    trdp::communication::ProcessDataHandler processHandler;
    trdp::communication::MessageDataHandler messageHandler;
    std::vector<ProcessDataMessage> pendingProcessData;
    std::vector<MessageDataMessage> pendingMessageData;
};

} // namespace

int main() {
    {
        auto adapter = std::make_shared<RecordingAdapter>();
        Wrapper wrapper{"loopback", adapter};

        std::vector<ProcessDataMessage> receivedPd;
        std::vector<MessageDataMessage> receivedMd;
        wrapper.registerProcessDataHandler([&receivedPd](const ProcessDataMessage &msg) { receivedPd.push_back(msg); });
        wrapper.registerMessageDataHandler([&receivedMd](const MessageDataMessage &msg) { receivedMd.push_back(msg); });

        wrapper.open();
        ProcessDataMessage pdMessage{"pd-event", 1001, 1001, {0x01, 0x02}};
        MessageDataMessage mdMessage{"md-event", 2001, 2001, {0x03}};
        wrapper.publishProcessData(pdMessage);
        const auto ack = wrapper.sendMessageData(mdMessage);
        wrapper.poll();
        wrapper.close();

        assert(!adapter->openCalled);
        assert(adapter->lastEndpoint == "loopback");
        assert(adapter->lastPd.label == "pd-event");
        assert(adapter->lastMd.label == "md-event");
        assert(ack.status == MessageDataStatus::Delivered);
        assert(ack.detail == "ack");
        assert(receivedPd.size() == 1);
        assert(receivedMd.size() == 1);

        const auto &diagnostics = wrapper.diagnostics();
        assert(diagnostics.size() == wrapper.telemetry().size());
        assert(diagnostics.size() == 6); // open, pd->, pd<-, md->, md<-, close
        for (const auto &event : diagnostics) {
            assert(event.level == DiagnosticEvent::Level::Info);
        }
    }

    {
        auto adapter = std::make_shared<RecordingAdapter>();
        Wrapper wrapper{"loopback", adapter};
        wrapper.open();
        adapter->failOnPd = true;
        bool caught = false;
        try {
            wrapper.publishProcessData({"pd-event", 1001, 1001, {}});
        } catch (const TrdpError &err) {
            caught = true;
            assert(err.errorCode() == 47);
            assert(std::string{err.what()} == "forced pd failure");
        }
        assert(caught);

        const auto &diagnostics = wrapper.diagnostics();
        assert(!diagnostics.empty());
        const auto &last = diagnostics.back();
        assert(last.level == DiagnosticEvent::Level::Error);

        adapter->failOnPoll = true;
        bool pollCaught = false;
        try {
            wrapper.poll();
        } catch (const TrdpError &) {
            pollCaught = true;
        }
        assert(pollCaught);

        bool closeCaught = false;
        try {
            wrapper.close();
        } catch (const TrdpError &) {
            closeCaught = true;
        }
        assert(!closeCaught);
    }

    {
        auto adapter = std::make_shared<RecordingAdapter>();
        Wrapper wrapper{"loopback", adapter};
        wrapper.registerProcessDataHandler([](const ProcessDataMessage &) {});
        wrapper.registerMessageDataHandler([](const MessageDataMessage &) {});
        wrapper.open();
        adapter->ackToReturn = {MessageDataStatus::Failed, "link down"};
        bool mdCaught = false;
        try {
            wrapper.sendMessageData({"md-event", 2001, 2001, {}});
        } catch (const TrdpError &) {
            mdCaught = true;
        }
        assert(!mdCaught);
        const auto &diagnostics = wrapper.diagnostics();
        assert(!diagnostics.empty());
        const auto &last = diagnostics.back();
        assert(last.message.find("failed") != std::string::npos);
        wrapper.close();
    }

    return 0;
}
