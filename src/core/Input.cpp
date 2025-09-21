#include "Input.hpp"

using namespace Hyprtoolkit;

Input::eMouseButton Input::buttonFromWayland(uint32_t wl) {
    switch (wl) {
        case 272: return MOUSE_BUTTON_LEFT;
        case 273: return MOUSE_BUTTON_RIGHT;
        case 274: return MOUSE_BUTTON_MIDDLE;
        default: return MOUSE_BUTTON_UNKNOWN;
    }
    return MOUSE_BUTTON_UNKNOWN;
}