#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    struct SNullData {
        CDynamicSize size{CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}};
    };

    struct SNullImpl {
        SNullData        data;

        WP<CNullElement> self;
    };
}