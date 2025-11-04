#pragma once

#include <string>

namespace trdp::communication {

struct DiagnosticEvent {
    enum class Level {
        Info,
        Error,
    };

    std::string timestamp;
    Level level{Level::Info};
    std::string message;
};

} // namespace trdp::communication
