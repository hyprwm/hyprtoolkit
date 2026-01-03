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

    enum eKeyboardModifier : uint8_t {
        HT_MODIFIER_SHIFT = (1 << 0),
        HT_MODIFIER_CAPS  = (1 << 1),
        HT_MODIFIER_CTRL  = (1 << 2),
        HT_MODIFIER_ALT   = (1 << 3),
        HT_MODIFIER_MOD2  = (1 << 4),
        HT_MODIFIER_MOD3  = (1 << 5),
        HT_MODIFIER_META  = (1 << 6),
        HT_MODIFIER_MOD5  = (1 << 7),

        HT_MODIFIER_CTRL_SHIFT = HT_MODIFIER_CTRL | HT_MODIFIER_SHIFT,
    };

    struct SKeyboardKeyEvent {
        uint32_t    xkbKeysym = 0;
        bool        down      = true;
        bool        repeat    = false;
        std::string utf8      = "";
        uint32_t    modMask   = 0; // eKeyboardModifier
    };
}
