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
#include "../core/CoreMacros.hpp"

namespace Hyprtoolkit {

    struct SElementInternalData;

    using colorFn = std::function<CHyprColor()>;

    class IElement {
      public:
        enum ePositionMode : uint8_t {
            HT_POSITION_ABSOLUTE = 0,
            HT_POSITION_AUTO,
        };

        enum ePositionFlag : uint8_t {
            HT_POSITION_FLAG_HCENTER = (1 << 0),
            HT_POSITION_FLAG_VCENTER = (1 << 1),
            HT_POSITION_FLAG_CENTER  = HT_POSITION_FLAG_HCENTER | HT_POSITION_FLAG_VCENTER,
            HT_POSITION_FLAG_LEFT    = (1 << 2),
            HT_POSITION_FLAG_RIGHT   = (1 << 3),
            HT_POSITION_FLAG_TOP     = (1 << 4),
            HT_POSITION_FLAG_BOTTOM  = (1 << 5),

            HT_POSITION_FLAG_ALL = 0xFF,
        };

        virtual ~IElement();

        virtual void                      paint() = 0;
        virtual Hyprutils::Math::Vector2D size()  = 0;
        virtual Hyprutils::Math::Vector2D posFromParent();
        virtual void                      reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize = {-1, -1});

        // TODO: move this to builders, this is clunky
        virtual void setPositionMode(ePositionMode mode);
        virtual void setPositionFlag(ePositionFlag flag, bool set);
        virtual void setAbsolutePosition(const Hyprutils::Math::Vector2D& offset);
        virtual void addChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void removeChild(Hyprutils::Memory::CSharedPointer<IElement> child);
        virtual void clearChildren();
        virtual void setMargin(float thick);
        virtual void setGrouped(bool grouped);

        // this will make this element get mouse input, then you can get events
        virtual void setReceivesMouse(bool x);
        virtual void setMouseEnter(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn);
        virtual void setMouseLeave(std::function<void()>&& fn);
        virtual void setMouseMove(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn);
        virtual void setMouseButton(std::function<void(Input::eMouseButton, bool)>&& fn);
        virtual void setMouseAxis(std::function<void(Input::eAxisAxis, float)>&& fn);

        virtual void setRepositioned(std::function<void()>&& fn);

        virtual void setGrow(bool grow);
        virtual void setGrow(bool growH, bool growV);

        HT_HIDDEN :

            /* Sizes for auto positioning in layouts */
            virtual std::optional<Hyprutils::Math::Vector2D>
                                                         preferredSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> minimumSize(const Hyprutils::Math::Vector2D& parent);
        virtual std::optional<Hyprutils::Math::Vector2D> maximumSize(const Hyprutils::Math::Vector2D& parent);

        virtual bool                                     acceptsMouseInput();
        virtual bool                                     acceptsKeyboardInput();
        virtual ePointerShape                            pointerShape();
        virtual bool                                     alwaysGetMouseInput();
        virtual void                                     imCommitNewText(const std::string& text);
        virtual void                                     imApplyText();

        virtual void                                     recheckColor();
        virtual bool                                     positioningDependsOnChild();
        virtual Hyprutils::Math::CBox                    opaqueBox();

        //

        Hyprutils::Memory::CUniquePointer<SElementInternalData> impl;

      protected:
        IElement();
    };
};
