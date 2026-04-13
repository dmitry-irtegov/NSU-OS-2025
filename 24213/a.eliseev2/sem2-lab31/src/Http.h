#pragma once

#include <cstdint>
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
    };
    static bool parse(const char **data, const char *end, Uri &uri, ParseInfo &info);

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
    };
    static bool parse(const char **data, const char *end, RequestLine &line, ParseInfo &info);

    RequestMethod method;
    Uri uri;
    Version version;
};

struct StatusLine {
    struct ParseInfo {
        const char *versionStart;
        const char *versionEnd;
        const char *codeStart;
        const char *codeEnd;
        const char *reasonStart;
        const char *reasonEnd;
    };
    static bool parse(const char **data, const char *end, StatusLine &line, ParseInfo &info);

    Version version;
    ResponseCode code;
};

} // namespace http
} // namespace proxy
