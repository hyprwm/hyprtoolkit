#pragma once

#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/types/PointerShape.hpp>

#include "../helpers/DamageRing.hpp"
#include "../helpers/Memory.hpp"
#include "../core/Input.hpp"

namespace Hyprtoolkit {

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
        virtual void damage(Hyprutils::Math::CRegion&& rg);
        virtual void scheduleFrame();
        virtual void damageEntire();

        virtual void onPreRender();
        virtual void render() = 0;
        virtual void scheduleReposition(WP<IElement> e);

        virtual void mouseEnter(const Hyprutils::Math::Vector2D& local);
        virtual void mouseMove(const Hyprutils::Math::Vector2D& local);
        virtual void mouseButton(const Input::eMouseButton button, bool state);
        virtual void mouseAxis(const Input::eAxisAxis axis, float delta);
        virtual void mouseLeave();

        virtual void keyboardKey(const Input::SKeyboardKeyEvent& e);
        virtual void unfocusKeyboard();

        virtual void updateFocus(const Hyprutils::Math::Vector2D& coords);
        virtual void setCursor(ePointerShape shape) = 0;

        void         initElementIfNeeded(SP<IElement>);

        // Damage ring is in pixel coords
        CDamageRing                        m_damageRing;
        bool                               m_needsFrame = true;
        WP<IToolkitWindow>                 m_self;
        Hyprutils::Math::Vector2D          m_mousePos;
        bool                               m_mouseIsDown = false;

        SP<SToolkitFocusLock>              m_mainHoverElement;
        std::vector<SP<SToolkitFocusLock>> m_hoveredElements;
        WP<IElement>                       m_keyboardFocus;

        std::vector<WP<IElement>>          m_needsReposition;
    };
}
