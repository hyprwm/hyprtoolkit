#pragma once

#include <string>

namespace Hyprtoolkit::UTF8 {
    size_t      length(const std::string&);
    size_t      offsetToUTF8Len(const std::string&, size_t offset);
    size_t      utf8ToOffset(const std::string&, size_t utf8);
    std::string substr(const std::string&, size_t start, size_t len = std::string::npos);
}
