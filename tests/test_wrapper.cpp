#include "trdp_simulator/communication/TrdpError.hpp"
#include "trdp_simulator/communication/Wrapper.hpp"

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

using trdp::communication::DiagnosticEvent;
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

    void publishProcessData(std::string_view label) override {
        if (!openCalled) {
            throw TrdpError("pd without open", 46, std::string{label});
        }
        lastPd = std::string{label};
        if (failOnPd) {
            throw TrdpError("forced pd failure", 47, lastPd);
        }
    }

    void sendMessageData(std::string_view label) override {
        if (!openCalled) {
            throw TrdpError("md without open", 48, std::string{label});
        }
        lastMd = std::string{label};
        if (failOnMd) {
            throw TrdpError("forced md failure", 49, lastMd);
        }
    }

    bool openCalled{false};
    bool failOnOpen{false};
    bool failOnClose{false};
    bool failOnPd{false};
    bool failOnMd{false};
    std::string lastEndpoint;
    std::string lastPd;
    std::string lastMd;
};

} // namespace

int main() {
    {
        auto adapter = std::make_shared<RecordingAdapter>();
        Wrapper wrapper{"loopback", adapter};
        wrapper.open();
        wrapper.publishProcessData("pd-event");
        wrapper.sendMessageData("md-event");
        wrapper.close();

        assert(!adapter->openCalled);
        assert(adapter->lastEndpoint == "loopback");
        assert(adapter->lastPd == "pd-event");
        assert(adapter->lastMd == "md-event");

        const auto &diagnostics = wrapper.diagnostics();
        assert(diagnostics.size() == wrapper.telemetry().size());
        assert(diagnostics.size() == 4); // open, pd, md, close
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
            wrapper.publishProcessData("pd-event");
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

        bool closeCaught = false;
        try {
            wrapper.close();
        } catch (const TrdpError &) {
            closeCaught = true;
        }
        // Close may throw because adapter is configured to fail (default false).
        assert(!closeCaught);
    }

    return 0;
}
