#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace proxy {
namespace http {

enum class ResponseCode {
    Ok = 200,
    Created = 201,
    Accepted = 202,
    NoContent = 204,
    MovedPermanently = 301,
    MovedTemporarily = 302,
    NotModified = 304,
    BadRequest = 400,
    Unauthorized = 401,
    Forbidden = 403,
    NotFound = 404,
    InternalServerError = 500,
    NotImplemented = 501,
    BadGateway = 502,
    ServiceUnavailable = 503,
};

enum class Version {
    Http10,
    Unknown,
};

enum class RequestMethod {
    Get,
    Head,
    Post,
};

struct Uri {
    struct ParseInfo {
        const char *start;
        const char *pathStart;
        const char *pathEnd;

        size_t hostSize() {
            return pathStart - start;
        }
    };
    static bool parse(const char **data, const char *end, Uri &uri,
                      ParseInfo &info);

    std::string raw;
    std::string host;
    uint16_t port;
};

struct RequestLine {
    struct ParseInfo {
        const char *methodStart;
        const char *methodEnd;
        Uri::ParseInfo uri;
        const char *versionStart;
        const char *versionEnd;
        const char *end;

        size_t size() {
            return end - methodStart;
        }
    };
    static bool parse(const char **data, const char *end, RequestLine &line,
                      ParseInfo &info);

    RequestMethod method;
    Uri uri;
    Version version;
};

struct Header {
    struct ParseInfo {
        const char *nameStart;
        const char *nameEnd;
        const char *valueStart;
        const char *valueEnd;
        const char *end;
        bool isEndOfHeaders;

        size_t size() {
            return end - nameStart;
        }
    };
    static bool parse(const char **data, const char *end, Header &header,
                      ParseInfo &info);

    std::string name;
    std::string value;

    bool nameEquals(const char *str) {
        return std::equal(name.begin(), name.end(), str, str + std::strlen(str),
                          [](unsigned char a, unsigned char b) {
                              return std::tolower(a) == std::tolower(b);
                          });
    }
};

struct StatusLine {
    struct ParseInfo {
        const char *versionStart;
        const char *versionEnd;
        const char *codeStart;
        const char *codeEnd;
        const char *reasonStart;
        const char *reasonEnd;
        const char *end;

        size_t size() {
            return end - versionStart;
        }
    };
    static bool parse(const char **data, const char *end, StatusLine &line,
                      ParseInfo &info);

    Version version;
    ResponseCode code;
};

} // namespace http
} // namespace proxy
