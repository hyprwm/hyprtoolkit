#pragma once

#include <hyprtoolkit/element/Rectangle.hpp>

#include "../../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {

    struct SRectangleData {
        colorFn      color           = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        int          rounding        = 0;
        colorFn      borderColor     = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        int          borderThickness = 0;
        CDynamicSize size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };
    struct SRectangleImpl {
        SRectangleData         data;

        PHLANIMVAR<CHyprColor> color;
        PHLANIMVAR<CHyprColor> borderColor;

        SP<CRectangleElement>  self;
    };
}
