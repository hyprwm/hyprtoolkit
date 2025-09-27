#pragma once

#include <cstdint>
#include <string>

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

    struct SKeyboardKeyEvent {
        // TODO: We use xkb internally, which will be a problem if we ever have a backend
        // that doesn't use xkb. No plans atm, but you know.
        uint32_t    xkbKeysym = 0;
        bool        down      = true;
        std::string utf8      = "";
    };

    eMouseButton buttonFromWayland(uint32_t wl);
}