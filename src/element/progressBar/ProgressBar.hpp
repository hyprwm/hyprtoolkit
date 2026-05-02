#pragma once

#include <hyprtoolkit/element/ProgressBar.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    struct SProgressBarData {
        float        value         = 0.F;
        bool         indeterminate = false;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, 16.F}};
    };

    struct SProgressBarImpl {
        SProgressBarData       data;

        WP<CProgressBarElement> self;

        SP<CRectangleElement>   background;
        SP<CRectangleElement>   foreground;

        void                    applyValue();
    };
}
