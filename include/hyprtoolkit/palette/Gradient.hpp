#pragma once

#include "Color.hpp"

#include <functional>
#include <vector>

namespace Hyprtoolkit {

    struct CGradientValueData {
        std::vector<CHyprColor> m_vColors;
        float                   m_fAngle = 0.F;

        CGradientValueData() = default;
        CGradientValueData(const CHyprColor& col) : m_vColors{col} {}

        bool operator==(const CGradientValueData& rhs) const;
    };

    using gradientFn = std::function<CGradientValueData()>;
}
