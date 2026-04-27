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
        return compareIgnoreCase(name.c_str(), str);
    }

    bool valueEquals(const char *str) {
        return compareIgnoreCase(value.c_str(), str);
    }

  private:
    static bool compareIgnoreCase(const char *str1, const char *str2) {
        return std::equal(str1, str1 + std::strlen(str1), str2,
                          str2 + std::strlen(str2),
                          [](unsigned char char1, unsigned char char2) {
                              return std::tolower(char1) == std::tolower(char2);
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

struct ChunkHeader {
    struct ParseInfo {
        const char *sizeStart;
        const char *sizeEnd;
        const char *end;
    };
    static bool parse(const char **data, const char *end, ChunkHeader &header,
                      ParseInfo &info);
    size_t chunkSize;
};

bool readChunkEnd(const char **data, const char *end);


} // namespace http
} // namespace proxy
