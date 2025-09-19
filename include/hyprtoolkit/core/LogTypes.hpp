#pragma once

#include <cstdint>

namespace Hyprtoolkit {
    enum eLogLevel : uint8_t {
        HT_LOG_TRACE = 0,
        HT_LOG_DEBUG,
        HT_LOG_WARNING,
        HT_LOG_ERROR,
        HT_LOG_CRITICAL,
    };
}
