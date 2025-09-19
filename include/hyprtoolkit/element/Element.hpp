#pragma once

#include <vector>
#include <cstdint>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/math/Box.hpp>

namespace Hyprtoolkit {
    class IElement {
      public:
        enum ePositionMode : uint8_t {
            HT_POSITION_ABSOLUTE = 0,
            HT_POSITION_CENTER,
            HT_POSITION_AUTO,
        };

        virtual ~IElement() = default;

        virtual void                      paint()                                      = 0;
        virtual void                      reposition(const Hyprutils::Math::CBox& box) = 0;
        virtual Hyprutils::Math::Vector2D size()                                       = 0;

        virtual void                      setPositionMode(ePositionMode mode);
        virtual void                      setAbsolutePosition(const Hyprutils::Math::Vector2D& offset);

        //
        std::vector<Hyprutils::Memory::CSharedPointer<IElement>> m_children;

        ePositionMode                                            m_positionMode = HT_POSITION_AUTO;
        Hyprutils::Math::Vector2D                                m_absoluteOffset;
    };
};
