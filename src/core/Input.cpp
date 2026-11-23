#include "Input.hpp"

using namespace Hyprtoolkit;

Input::eMouseButton Input::buttonFromWayland(uint32_t wl) {
    switch (wl) {
        case 272: return MOUSE_BUTTON_LEFT;
        case 273: return MOUSE_BUTTON_RIGHT;
        case 274: return MOUSE_BUTTON_MIDDLE;
        // BTN_SIDE and BTN_EXTRA on most mice, BTN_BACK and BTN_FORWARD on some
        case 275:
        case 278: return MOUSE_BUTTON_BACK;
        case 276:
        case 277: return MOUSE_BUTTON_FORWARD;
        default: return MOUSE_BUTTON_UNKNOWN;
    }
    return MOUSE_BUTTON_UNKNOWN;
}