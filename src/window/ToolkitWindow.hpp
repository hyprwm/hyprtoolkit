#pragma once

#include <hyprtoolkit/window/Window.hpp>

#include "../helpers/DamageRing.hpp"
#include "../helpers/Memory.hpp"

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

        // Damage ring is in pixel coords
        CDamageRing               m_damageRing;
        bool                      m_needsFrame = true;
        WP<IToolkitWindow>        m_self;

        std::vector<WP<IElement>> m_needsReposition;
    };
}
