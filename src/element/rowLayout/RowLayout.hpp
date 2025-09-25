#include <hyprtoolkit/element/RowLayout.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SRowLayoutData {
        CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
        size_t       gap  = 0;
    };

    struct SRowLayoutImpl {
        SRowLayoutData        data;

        WP<CRowLayoutElement> self;
    };
}
