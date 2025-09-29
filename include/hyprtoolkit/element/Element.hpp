#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include <functional>

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/WeakPtr.hpp>
#include <hyprutils/memory/UniquePtr.hpp>
#include <hyprutils/math/Box.hpp>

#include "../types/PointerShape.hpp"
#include "../palette/Color.hpp"
#include "../core/Input.hpp"

namespace Hyprtoolkit {

    struct SElementInternalData;

    using colorFn = std::function<CHyprColor()>;

    class IElement {
      public:
        enum ePositionMode : uint8_t {
            HT_POSITION_ABSOLUTE = 0,
            HT_POSITION_CENTER,
            HT_POSITION_HCENTER,
            HT_POSITION_VCENTER,
            HT_POSITION_RIGHT,
            HT_POSITION_LEFT,
            HT_POSITION_AUTO,
        };

        virtual ~IElement();

        virtual void                      paint() = 0;
        virtual Hyprutils::Math::Vector2D size()  = 0;
        virtual void                      reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});

        // TODO: move this to builders, this is clunky
        virtual void setPositionMode(ePositionMode mode);
        virtual void setAbsolutePosition(const Hyprutils::Math::Vector2D& offset);
        virtual void addChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void removeChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void clearChildren();
        virtual void setMargin(float thick);

        // this will make this element get mouse input, then you can get events
        virtual void setReceivesMouse(bool x);
        virtual void setMouseEnter(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn);
        virtual void setMouseLeave(std::function<void()>&& fn);
        virtual void setMouseMove(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn);
        virtual void setMouseButton(std::function<void(Input::eMouseButton, bool)>&& fn);
        virtual void setMouseAxis(std::function<void(Input::eAxisAxis, float)>&& fn);

        /* Sizes for auto positioning in layouts */
        virtual std::optional<Hyprutils::Math::Vector2D> preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);
        virtual void                                     setGrow(bool grow);
        virtual void                                     setGrow(bool growH, bool growV);

        virtual bool                                     acceptsMouseInput();
        virtual bool                                     acceptsKeyboardInput();
        virtual ePointerShape                            pointerShape();
        virtual bool                                     alwaysGetMouseInput();
        virtual void                                     imCommitNewText(const std::string& text);
        virtual void                                     imApplyText();

        virtual void                                     recheckColor();

        //

        Hyprutils::Memory::CUniquePointer<SElementInternalData> impl;

      protected:
        IElement();
    };
};
