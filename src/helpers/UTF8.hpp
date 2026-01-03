#pragma once

#include <string>

namespace Hyprtoolkit::UTF8 {
    size_t      codepointLen(const char* utf8Char, size_t max);
    size_t      codepointLenBefore(const std::string& s, size_t offset);
    size_t      length(const std::string&);
    size_t      offsetToUTF8Len(const std::string&, size_t offset);
    size_t      utf8ToOffset(const std::string&, size_t utf8);
    std::string substr(const std::string&, size_t start, size_t len = std::string::npos);
    size_t      findFirstOf(const std::string&, const std::string ch, size_t offset = 0);
    size_t      findLastOf(const std::string&, const std::string ch, size_t offset = std::string::npos);
    size_t      findFirstNotOf(const std::string&, const std::string ch, size_t offset = 0);
    size_t      findLastNotOf(const std::string&, const std::string ch, size_t offset = std::string::npos);
}
