#pragma once

#include <hyprutils/math/Box.hpp>
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IElement;

    class CPositioner {
      public:
        void position(SP<IElement> element, const Hyprutils::Math::CBox& box);
    };

    inline UP<CPositioner> g_positioner = makeUnique<CPositioner>();
}