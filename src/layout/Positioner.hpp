#pragma once

#include <hyprutils/math/Box.hpp>
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IElement;

    class CPositioner {
      public:
        void position(SP<IElement> element, const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        void repositionNeeded(SP<IElement> element);

      private:
        size_t m_depth = 0;
    };

    inline UP<CPositioner> g_positioner = makeUnique<CPositioner>();
}