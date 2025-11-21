#include "UTF8.hpp"

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::UTF8;

static size_t codepointLen(const char* utf8Char, size_t max) {
    size_t len = 1;
    while (len < max) {
        if (((*(utf8Char + len)) & 0xC0) == 0x80) {
            len++;
            continue;
        }

        break;
    }

    return len;
}

size_t UTF8::length(const std::string& s) {
    if (s.empty())
        return 0;

    const char* c    = s.c_str();
    size_t      len  = 0;
    auto        endp = s.c_str() + s.length();
    while (c < endp) {
        c += codepointLen(c, (endp - c));
        len++;
    }
    return len;
}

size_t UTF8::offsetToUTF8Len(const std::string& s, size_t offset) {
    if (s.empty())
        return 0;

    const char* c    = s.c_str();
    size_t      len  = 0;
    auto        endp = std::min(s.c_str() + s.length(), s.c_str() + offset);
    while (c < endp) {
        c += codepointLen(c, (endp - c));
        len++;
    }
    return len;
}

size_t UTF8::utf8ToOffset(const std::string& s, size_t utf8) {
    if (s.empty())
        return 0;

    const char* c    = s.c_str();
    size_t      len  = 0;
    auto        endp = s.c_str() + s.length();
    while (c < endp) {
        if (len >= utf8)
            return c - s.c_str();

        c += codepointLen(c, (endp - c));
        len++;
    }
    return s.size();
}

std::string UTF8::substr(const std::string& s, size_t start, size_t length) {
    if (s.empty() || length == 0)
        return "";

    bool        started   = false;
    size_t      byteStart = 0, byteEnd = std::string::npos;

    const char* c    = s.c_str();
    size_t      len  = 0;
    auto        endp = s.c_str() + s.length();
    while (c < endp) {
        if (len >= start && !started) {
            started   = true;
            byteStart = c - s.c_str();

            if (length == std::string::npos)
                break;

            len = 0;
        }

        c += codepointLen(c, endp - c);

        len++;

        if (len >= length && started) {
            byteEnd = c - s.c_str();
            break;
        }
    }

    if (len == 0 || !started)
        return "";

    if (byteEnd == std::string::npos)
        return s.substr(byteStart);

    return s.substr(byteStart, byteEnd - byteStart);
}
