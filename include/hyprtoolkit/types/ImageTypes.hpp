
#pragma once

#include <cstdint>

namespace Hyprtoolkit {
    enum eImageFitMode : uint8_t {
        IMAGE_FIT_MODE_STRETCH = 0,
        IMAGE_FIT_MODE_COVER,
        IMAGE_FIT_MODE_CONTAIN,
        IMAGE_FIT_MODE_TILE,
    };
}
