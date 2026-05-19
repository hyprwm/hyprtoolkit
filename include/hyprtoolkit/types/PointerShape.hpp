#pragma once

#include <cstdint>

namespace Hyprtoolkit {
    enum ePointerShape : uint8_t {
        HT_POINTER_ARROW = 0,
        HT_POINTER_POINTER,
        HT_POINTER_TEXT,
        HT_POINTER_RESIZE_NS,
        HT_POINTER_RESIZE_EW,
        HT_POINTER_RESIZE_NESW,
        HT_POINTER_RESIZE_NWSE,
    };
}