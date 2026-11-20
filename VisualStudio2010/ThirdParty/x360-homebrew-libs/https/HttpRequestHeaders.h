#ifndef NUVIO_HTTP_REQUEST_HEADERS_H
#define NUVIO_HTTP_REQUEST_HEADERS_H
#include <string>
#include <cstdio>
#include <cstring>

namespace Http {
// Keep a caller-supplied Accept field. Sending */* followed by application/json
// makes PostgREST negotiate a combined media type and can produce HTTP 406.
inline std::string RequestHeaders(const char *method, const std::string &path,
                                 const std::string &host, std::string extra,
                                 size_t bodySize) {
    bool hasAccept = false;
    for (size_t start = 0; start < extra.size();) {
        const size_t end = extra.find('\n', start);
        std::string line = extra.substr(start, end == std::string::npos ? end : end - start);
        for (size_t i = 0; i < line.size(); ++i)
            if (line[i] >= 'A' && line[i] <= 'Z') line[i] += 'a' - 'A';
        if (line.compare(0, 7, "accept:") == 0) hasAccept = true;
        if (end == std::string::npos) break;
        start = end + 1;
    }
    if (!extra.empty() && extra[extra.size() - 1] != '\n') extra += "\r\n";
    std::string result = std::string(method) + " " + path + " HTTP/1.1\r\nHost: " + host +
        "\r\nConnection: close\r\n";
    if (!hasAccept) result += "Accept: */*\r\n";
    result += extra;
    if (bodySize || std::strcmp(method, "POST") == 0) {
        char length[64]; std::sprintf(length, "Content-Length: %u\r\n", (unsigned)bodySize);
        result += length;
    }
    return result + "\r\n";
}
}
#endif
