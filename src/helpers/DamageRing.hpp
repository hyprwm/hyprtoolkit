#pragma once

#include <hyprutils/math/Region.hpp>
#include <array>

namespace Hyprtoolkit {
    constexpr static int DAMAGE_RING_PREVIOUS_LEN = 2;

    class CDamageRing {
      public:
        void                     setSize(const Hyprutils::Math::Vector2D& size_);
        bool                     damage(Hyprutils::Math::CRegion&& rg);
        void                     damageEntire();
        void                     rotate();
        Hyprutils::Math::CRegion getBufferDamage(int age);
        bool                     hasChanged();

      private:
        Hyprutils::Math::Vector2D                                      m_size;
        Hyprutils::Math::CRegion                                       m_current;
        std::array<Hyprutils::Math::CRegion, DAMAGE_RING_PREVIOUS_LEN> m_previous;
        size_t                                                         m_previousIdx = 0;
    };
}
