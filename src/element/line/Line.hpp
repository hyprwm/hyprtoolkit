#pragma once

#include <hyprtoolkit/element/Line.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {

    struct SLineData {
        colorFn                                color = [] { return CHyprColor{1.F, 1.F, 1.F, 1.F}; };
        int                                    thick = 2;
        std::vector<Hyprutils::Math::Vector2D> points;
        CDynamicSize                           size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };
    struct SLineImpl {
        SLineData        data;

        WP<CLineElement> self;
    };
}
