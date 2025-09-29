#pragma once

#include <cstdint>
#include <string>

#include <hyprtoolkit/core/Input.hpp>

namespace Hyprtoolkit::Input {
    struct SKeyboardKeyEvent {
        // TODO: We use xkb internally, which will be a problem if we ever have a backend
        // that doesn't use xkb. No plans atm, but you know.
        uint32_t    xkbKeysym = 0;
        bool        down      = true;
        std::string utf8      = "";
    };

    eMouseButton buttonFromWayland(uint32_t wl);
}