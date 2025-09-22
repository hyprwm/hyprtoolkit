#pragma once

#include <hyprtoolkit/element/Rectangle.hpp>

#include "../core/AnimatedVariable.hpp"

namespace Hyprtoolkit {
    struct SRectangleImpl {
        SRectangleData         data;

        PHLANIMVAR<CHyprColor> color;
        PHLANIMVAR<CHyprColor> borderColor;
    };
}
