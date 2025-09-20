#pragma once

#include <vector>
#include <cstdint>
#include <optional>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/math/Box.hpp>

namespace Hyprtoolkit {

    struct SElementInternalData;

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
        virtual void                      addChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void                      clearChildren();

        /* Sizes for auto positioning in layouts */
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual void                                     setGrow(bool grow);

        //

        Hyprutils::Memory::CUniquePointer<SElementInternalData> m_elementData;
        Hyprutils::Memory::CWeakPointer<IElement>               m_self;
        Hyprutils::Math::CBox                                   m_position;

      protected:
        IElement();
    };
};
