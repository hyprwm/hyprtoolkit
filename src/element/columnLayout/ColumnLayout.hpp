#include <hyprtoolkit/element/ColumnLayout.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SColumnLayoutData {
        CDynamicSize size = CDynamicSize(CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1});
        size_t       gap  = 0;
    };

    struct SColumnLayoutImpl {
        SColumnLayoutData        data;

        WP<CColumnLayoutElement> self;
    };
}
