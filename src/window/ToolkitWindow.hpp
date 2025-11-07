#pragma once

#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/types/PointerShape.hpp>

#include "../helpers/DamageRing.hpp"
#include "../helpers/Memory.hpp"
#include "../core/Input.hpp"

#include "Window.hpp"

namespace Hyprtoolkit {

    class CRectangleElement;
    class CTextElement;
    class CTimer;

    struct SToolkitFocusLock {
        SToolkitFocusLock(SP<IElement> e, const Hyprutils::Math::Vector2D& coord);
        ~SToolkitFocusLock();

        WP<IElement> m_el;
    };

    class IToolkitWindow : public IWindow {
      public:
        IToolkitWindow()          = default;
        virtual ~IToolkitWindow() = default;

        /*
            Schedules a frame event as well.
            Takes logical coordinates (unscaled)
        */
        virtual void                      damage(Hyprutils::Math::CRegion&& rg);
        virtual void                      scheduleFrame();
        virtual void                      damageEntire();

        virtual Hyprutils::Math::Vector2D cursorPos();
        virtual void                      onPreRender();
        virtual void                      render() = 0;
        virtual void                      scheduleReposition(WP<IElement> e);

        virtual SP<IWindow>               openPopup(const SWindowCreationData& data) = 0;

        virtual void                      mouseEnter(const Hyprutils::Math::Vector2D& local);
        virtual void                      mouseMove(const Hyprutils::Math::Vector2D& local);
        virtual void                      mouseButton(const Input::eMouseButton button, bool state);
        virtual void                      mouseAxis(const Input::eAxisAxis axis, float delta);
        virtual void                      mouseLeave();

        virtual void                      keyboardKey(const Input::SKeyboardKeyEvent& e);
        virtual void                      unfocusKeyboard();
        virtual void                      setKeyboardFocus(SP<IElement>);

        virtual void                      updateFocus(const Hyprutils::Math::Vector2D& coords);
        virtual void                      setCursor(ePointerShape shape) = 0;

        virtual void                      setIMTo(const Hyprutils::Math::CBox& box, const std::string& str, size_t cursor);
        virtual void                      resetIM();

        virtual void                      openTooltip(const std::string& s, const Hyprutils::Math::Vector2D& pos);
        virtual void                      closeTooltip();

        void                              initElementIfNeeded(SP<IElement>);

        // Damage ring is in pixel coords
        CDamageRing                        m_damageRing;
        bool                               m_needsFrame = true;
        WP<IToolkitWindow>                 m_self;
        Hyprutils::Math::Vector2D          m_mousePos;
        bool                               m_mouseIsDown = false;

        std::string                        m_currentInput       = "";
        size_t                             m_currentInputCursor = 0;

        SP<SToolkitFocusLock>              m_mainHoverElement;
        std::vector<SP<SToolkitFocusLock>> m_hoveredElements;
        WP<IElement>                       m_keyboardFocus;
        bool                               m_scheduledRender = false;

        std::vector<WP<IElement>>          m_needsReposition;

        struct {
            SP<IToolkitWindow>    tooltipPopup;
            SP<CRectangleElement> bg;
            SP<CTextElement>      text;
            ASP<CTimer>           hoverTooltipTimer;
        } m_tooltip;
    };
}
