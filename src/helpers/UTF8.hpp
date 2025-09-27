#pragma once

#include <string>

namespace Hyprtoolkit::UTF8 {
    size_t      length(const std::string&);
    std::string substr(const std::string&, size_t start, size_t len = std::string::npos);
}
