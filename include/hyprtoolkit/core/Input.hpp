#pragma once

#include <cstdint>

namespace Hyprtoolkit::Input {
    enum eMouseButton : uint8_t {
        MOUSE_BUTTON_UNKNOWN,
        MOUSE_BUTTON_LEFT,
        MOUSE_BUTTON_RIGHT,
        MOUSE_BUTTON_MIDDLE,
    };

    enum eAxisAxis : uint8_t {
        AXIS_AXIS_HORIZONTAL,
        AXIS_AXIS_VERTICAL,
    };
}