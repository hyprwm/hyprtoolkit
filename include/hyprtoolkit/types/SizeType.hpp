#pragma once

#include <cstdint>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class CDynamicSize {
      public:
        enum eSizingType : uint8_t {
            HT_SIZE_ABSOLUTE,
            HT_SIZE_PERCENT,
            HT_SIZE_AUTO, // contain child(ren)
        };

        CDynamicSize(eSizingType typeX, eSizingType typeY, const Hyprutils::Math::Vector2D& size);

        Hyprutils::Math::Vector2D calculate(Hyprutils::Math::Vector2D elSize) const;

      private:
        eSizingType               m_typeX = HT_SIZE_ABSOLUTE, m_typeY = HT_SIZE_ABSOLUTE;
        Hyprutils::Math::Vector2D m_value;
    };
}