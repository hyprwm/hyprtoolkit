#pragma once

#include <hyprutils/math/Box.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IElement;

    struct SPositionerData {
        Hyprutils::Math::CBox baseBox;
    };

    struct SRepositionData {
        Hyprutils::Math::Vector2D offset = {0, 0};
        bool                      growX  = false;
        bool                      growY  = false;
    };

    class CPositioner {
      public:
        void position(SP<IElement> element, const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});
        void positionChildren(SP<IElement> element, const SRepositionData& data = {});
        void repositionNeeded(SP<IElement> element);

      private:
        void   initElementIfNeeded(SP<IElement> element);

        size_t m_depth = 0;
    };

    inline UP<CPositioner> g_positioner = makeUnique<CPositioner>();
}