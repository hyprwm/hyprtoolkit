#pragma once

#include <cstdint>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class CFontSize {
      public:
        enum eSizingBase : uint8_t {
            HT_FONT_H1,
            HT_FONT_H2,
            HT_FONT_H3,
            HT_FONT_TEXT,
            HT_FONT_SMALL,
            HT_FONT_ABSOLUTE,
        };

        // in the case of ABSOLUTE, multiplier is the raw value.
        CFontSize(eSizingBase base, float multiplier = 1.F);

        float ptSize();

      private:
        eSizingBase m_base  = HT_FONT_TEXT;
        float       m_value = 1.F;
    };

    enum eFontAlignment : uint8_t {
        HT_FONT_ALIGN_LEFT,
        HT_FONT_ALIGN_CENTER,
        HT_FONT_ALIGN_RIGHT,
    };
}