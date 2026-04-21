#include "Http.h"
#include <cstring>
#include <stdexcept>

namespace proxy {
namespace http {

static bool partialHasPrefix(const char *str1, const char *str2,
                             size_t length = SIZE_MAX) {
    size_t minLen = std::min(::strlen(str1), ::strlen(str2));
    return std::strncmp(str1, str2, std::min(minLen, length)) == 0;
}

static bool findChar(const char **ptr, const char *end, const char *chars) {
    while (*ptr < end) {
        char ch = **ptr;
        if (std::strchr(chars, ch)) {
            break;
        }
        if (!std::isspace(ch) && !std::isgraph(ch)) {
            throw std::runtime_error(
                "Invalid character in request/status line");
        }
        (*ptr)++;
    }
    if (*ptr == end) {
        return false;
    }
    return true;
}

static bool readChar(const char **ptr, const char *end, const char *chars) {
    if (*ptr == end) {
        return false;
    }
    char ch = **ptr;
    if (!std::strchr(chars, ch)) {
        throw std::runtime_error("Invalid character. Expected: " +
                                 std::string(chars));
    }
    (*ptr)++;
    return true;
}

static bool readEol(const char **ptr, const char *end) {
    if (*ptr == end) {
        return false;
    }
    if (**ptr == '\n') {
        (*ptr)++;
        return true;
    }
    if (**ptr != '\r') {
        throw std::runtime_error("Invalid line terminator. CR expected.");
    }
    (*ptr)++;
    if (*ptr == end) {
        return false;
    }
    if (**ptr != '\n') {
        throw std::runtime_error("Invalid line terminator. LF expected.");
    }
    (*ptr)++;
    return true;
}

static bool parseWord(const char **data, const char *end, std::string &word,
                      const char *separators = " \n\r") {
    const char *start = *data;
    if (!findChar(data, end, separators)) {
        return false;
    }
    word = std::string(start, *data);
    return true;
}

static bool parseAbsolute(const char **data, const char *end, Uri &uri) {
    if (!partialHasPrefix(*data, "http://", end - *data)) {
        throw std::runtime_error("Invalid URI scheme");
    }
    if (end - *data < 7) {
        return false;
    }
    (*data) += 7;

    const char *hostStart = *data;
    while (*data < end) {
        char ch = **data;
        if (!std::isgraph(ch)) {
            throw std::runtime_error("Invalid character in URI host");
        }
        if (ch == ':' || ch == '/') {
            break;
        }
        (*data)++;
    }
    if (*data == end) {
        return false;
    }
    uri.host = std::string(hostStart, *data);

    if (**data == ':') {
        (*data)++;
        const char *portStart = *data;
        while (*data < end) {
            char ch = **data;
            if (ch == '/') {
                break;
            }
            if (!std::isdigit(ch)) {
                throw std::runtime_error("Invalid character in URI port");
            }
            (*data)++;
        }
        if (*data == end) {
            return false;
        }
        long port = std::stol(std::string(portStart, *data));
        if (port < 0 && port > static_cast<long>(UINT16_MAX)) {
            throw std::runtime_error("Invalid URI port");
        }
        uri.port = static_cast<uint16_t>(port);
    } else {
        uri.port = 80;
    }
    return true;
}

bool Uri::parse(const char **data, const char *end, Uri &uri,
                ParseInfo &parseInfo) {
    if (*data == end) {
        return false;
    }

    uri.host = std::string();
    uri.port = 0;

    parseInfo.start = *data;
    if (**data != '/' && !parseAbsolute(data, end, uri)) {
        return false;
    }
    parseInfo.pathStart = *data;
    if (!findChar(data, end, " \n\r")) {
        return false;
    }
    parseInfo.pathEnd = *data;
    uri.raw = std::string(parseInfo.start, *data);
    return true;
}

static bool parseMethod(const char **data, const char *end,
                        RequestMethod &method) {
    std::string strMethod;
    if (!parseWord(data, end, strMethod)) {
        return false;
    }
    if (strMethod == "GET") {
        method = RequestMethod::Get;
    } else if (strMethod == "POST") {
        method = RequestMethod::Post;
    } else if (strMethod == "HEAD") {
        method = RequestMethod::Head;
    } else {
        throw std::runtime_error("Invalid request method: " + strMethod);
    }
    return true;
}

static bool parseVersion(const char **data, const char *end, Version &version) {
    std::string strVersion;
    if (!parseWord(data, end, strVersion)) {
        return false;
    }
    if (strVersion == "HTTP/1.0") {
        version = Version::Http10;
    } else if (strVersion.size() == 8 &&
               partialHasPrefix(strVersion.c_str(), "HTTP/")) {
        version = Version::Unknown;
    } else {
        throw std::runtime_error("Invalid HTTP version");
    }
    return true;
}

bool RequestLine::parse(const char **data, const char *end, RequestLine &line,
                        ParseInfo &parseInfo) {
    parseInfo.methodStart = *data;
    if (!parseMethod(data, end, line.method)) {
        return false;
    }
    parseInfo.methodEnd = *data;
    if (!readChar(data, end, " ")) {
        return false;
    }
    if (!Uri::parse(data, end, line.uri, parseInfo.uri)) {
        return false;
    }
    if (!readChar(data, end, " ")) {
        return false;
    }
    parseInfo.versionStart = *data;
    if (!parseVersion(data, end, line.version)) {
        return false;
    }
    parseInfo.versionEnd = *data;
    if (!readEol(data, end)) {
        return false;
    }
    parseInfo.end = *data;
    return true;
}

static bool parseResponseCode(const char **data, const char *end,
                              ResponseCode &code) {
    std::string strCode;
    if (!parseWord(data, end, strCode)) {
        return false;
    }
    long numCode = std::stol(strCode);
    if (numCode < 200 && numCode > 503) {
        throw std::runtime_error("Invalid status code");
    }
    code = static_cast<ResponseCode>(numCode);
    return true;
}

bool StatusLine::parse(const char **data, const char *end, StatusLine &line,
                       ParseInfo &info) {
    info.versionStart = *data;
    if (!parseVersion(data, end, line.version)) {
        return false;
    }
    info.versionEnd = *data;
    if (!readChar(data, end, " ")) {
        return false;
    }
    info.codeStart = *data;
    if (!parseResponseCode(data, end, line.code)) {
        return false;
    }
    info.codeEnd = *data;
    if (!readChar(data, end, " ")) {
        return false;
    }
    info.reasonStart = *data;
    if (!findChar(data, end, "\r\n")) {
        return false;
    }
    info.reasonEnd = *data;
    if (!readEol(data, end)) {
        return false;
    }
    info.end = *data;
    return true;
}

bool Header::parse(const char **data, const char *end, Header &header,
                   ParseInfo &info) {
    if (*data == end) {
        return false;
    }
    if (**data == '\r' || **data == '\n') {
        info.nameStart = *data;
        if (!readEol(data, end)) {
            return false;
        }
        info.isEndOfHeaders = true;
        return true;
    }

    info.isEndOfHeaders = false;
    info.nameStart = *data;
    if (!parseWord(data, end, header.name, ":")) {
        return false;
    }
    info.nameEnd = *data;
    if (!readChar(data, end, ":")) {
        return false;
    }
    if (!readChar(data, end, " ")) {
        return false;
    }
    info.valueStart = *data;
    while (true) {
        if (!findChar(data, end, "\r\n")) {
            return false;
        }
        info.valueEnd = *data;
        if (!readEol(data, end)) {
            return false;
        }
        if (*data == end) {
            return false;
        }
        if (**data != ' ' && **data != '\t') {
            break;
        }
        (*data)++;
    }
    header.name = std::string(info.nameStart, info.nameEnd);
    header.value = std::string(info.valueStart, info.valueEnd);
    info.end = *data;
    return true;
}

} // namespace http
} // namespace proxy
