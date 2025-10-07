#pragma once

#include <cstdint>
#include <string>

#include <hyprtoolkit/core/Input.hpp>

namespace Hyprtoolkit::Input {
    eMouseButton buttonFromWayland(uint32_t wl);
}