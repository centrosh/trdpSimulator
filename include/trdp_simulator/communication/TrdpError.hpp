#pragma once

#include <stdexcept>
#include <string>
#include <utility>

namespace trdp::communication {

/**
 * @brief Exception representing an error returned by the underlying TRDP stack.
 */
class TrdpError : public std::runtime_error {
public:
    TrdpError(std::string message, int errorCode, std::string context = {})
        : std::runtime_error(std::move(message)), m_errorCode(errorCode), m_context(std::move(context)) {}

    [[nodiscard]] int errorCode() const noexcept { return m_errorCode; }
    [[nodiscard]] const std::string &context() const noexcept { return m_context; }

private:
    int m_errorCode;
    std::string m_context;
};

} // namespace trdp::communication
