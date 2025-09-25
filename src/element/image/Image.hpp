#include <hyprtoolkit/element/Image.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SImageData {
        std::string  path;
        float        a        = 1.F;
        int          rounding = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };

    struct SImageImpl {
        SImageData        data;

        WP<CImageElement> self;
    };
}
