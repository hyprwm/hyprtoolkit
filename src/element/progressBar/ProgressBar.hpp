#pragma once

#include <hyprtoolkit/element/ProgressBar.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/element/Null.hpp>

#include "../../core/AnimatedVariable.hpp"
#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    struct SProgressBarData {
        float        value         = 0.F;
        bool         indeterminate = false;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, 16.F}};
    };

    // pulse rectangle width as a fraction of the bar
    static constexpr float PROGRESSBAR_PULSE_WIDTH = 0.3F;

    struct SProgressBarImpl {
        SProgressBarData        data;

        WP<CProgressBarElement> self;

        SP<CRectangleElement>   background;
        SP<CRectangleElement>   foreground;

        // 0..1 cycling phase used for the indeterminate pulse
        PHLANIMVAR<float>       phase;

        void                    applyValue();
        void                    startIndeterminate();
        void                    stopIndeterminate();
        void                    onPulseTick();
    };
}
