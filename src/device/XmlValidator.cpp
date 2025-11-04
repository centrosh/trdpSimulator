#include "trdp_simulator/device/XmlValidator.hpp"

#include <libxml/xmlschemas.h>
#include <libxml/xmlstring.h>

#include <sstream>
#include <stdexcept>

namespace trdp::device {

namespace {

void collectError(void *ctx, const char *message, ...) {
    auto *stream = static_cast<std::ostringstream *>(ctx);
    va_list args;
    va_start(args, message);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), message, args);
    va_end(args);
    (*stream) << buffer;
}

} // namespace

XmlValidator::XmlValidator(std::filesystem::path schemaPath) : m_schemaPath(std::move(schemaPath)) {
    if (!std::filesystem::exists(m_schemaPath)) {
        throw std::invalid_argument("Schema file not found: " + m_schemaPath.string());
    }
}

const std::filesystem::path &XmlValidator::schemaPath() const noexcept {
    return m_schemaPath;
}

XmlValidationResult XmlValidator::validate(const std::filesystem::path &xmlPath) const {
    if (!std::filesystem::exists(xmlPath)) {
        return {false, "XML file not found: " + xmlPath.string()};
    }

    xmlSchemaParserCtxtPtr parser = xmlSchemaNewParserCtxt(m_schemaPath.c_str());
    if (parser == nullptr) {
        throw std::runtime_error("Failed to create XML schema parser context");
    }

    xmlSchemaPtr schema = xmlSchemaParse(parser);
    xmlSchemaFreeParserCtxt(parser);
    if (schema == nullptr) {
        throw std::runtime_error("Failed to parse XML schema");
    }

    xmlDocPtr doc = xmlReadFile(xmlPath.c_str(), nullptr, 0);
    if (doc == nullptr) {
        xmlSchemaFree(schema);
        return {false, "Unable to parse XML file"};
    }

    xmlSchemaValidCtxtPtr validCtxt = xmlSchemaNewValidCtxt(schema);
    if (validCtxt == nullptr) {
        xmlFreeDoc(doc);
        xmlSchemaFree(schema);
        throw std::runtime_error("Failed to create XML validation context");
    }

    std::ostringstream errors;
    xmlSchemaSetValidErrors(validCtxt, collectError, collectError, &errors);

    const int result = xmlSchemaValidateDoc(validCtxt, doc);

    xmlSchemaFreeValidCtxt(validCtxt);
    xmlFreeDoc(doc);
    xmlSchemaFree(schema);

    if (result == 0) {
        return {true, {}};
    }

    std::string message = errors.str();
    if (message.empty()) {
        message = "XML validation failed";
    }
    return {false, message};
}

} // namespace trdp::device

