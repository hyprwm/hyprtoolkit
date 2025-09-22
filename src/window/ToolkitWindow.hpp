#pragma once

#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/types/PointerShape.hpp>

#include "../helpers/DamageRing.hpp"
#include "../helpers/Memory.hpp"
#include "../core/Input.hpp"

namespace Hyprtoolkit {

    class IToolkitWindow : public IWindow {
      public:
        IToolkitWindow()          = default;
        virtual ~IToolkitWindow() = default;

        /*
            Schedules a frame event as well.
            Takes logical coordinates (unscaled)
        */
        virtual void damage(const Hyprutils::Math::CRegion& rg);
        virtual void scheduleFrame();
        virtual void damageEntire();

        virtual void onPreRender();
        virtual void render() = 0;
        virtual void scheduleReposition(WP<IElement> e);

        virtual void mouseEnter(const Hyprutils::Math::Vector2D& local);
        virtual void mouseMove(const Hyprutils::Math::Vector2D& local);
        virtual void mouseButton(const Input::eMouseButton button, bool state);
        virtual void mouseLeave();

        virtual void updateFocus(const Hyprutils::Math::Vector2D& coords);
        virtual void setCursor(ePointerShape shape) = 0;

        // Damage ring is in pixel coords
        CDamageRing               m_damageRing;
        bool                      m_needsFrame = true;
        WP<IToolkitWindow>        m_self;
        Hyprutils::Math::Vector2D m_mousePos;

        SP<IElement>              m_hoveredElement;

        std::vector<WP<IElement>> m_needsReposition;
    };
}
