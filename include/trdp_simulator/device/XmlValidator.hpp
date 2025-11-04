#pragma once

#include <filesystem>
#include <string>

namespace trdp::device {

struct XmlValidationResult {
    bool success{false};
    std::string message;
};

class XmlValidator {
public:
    explicit XmlValidator(std::filesystem::path schemaPath);

    [[nodiscard]] const std::filesystem::path &schemaPath() const noexcept;
    [[nodiscard]] XmlValidationResult validate(const std::filesystem::path &xmlPath) const;

private:
    std::filesystem::path m_schemaPath;
};

} // namespace trdp::device

