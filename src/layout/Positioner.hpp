#pragma once

#include <hyprutils/math/Box.hpp>
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IElement;

    class CPositioner {
      public:
        void position(SP<IElement> element, const Hyprutils::Math::CBox& box);
        void repositionNeeded(SP<IElement> element);

      private:
        size_t m_depth = 0;
    };

    inline UP<CPositioner> g_positioner = makeUnique<CPositioner>();
}