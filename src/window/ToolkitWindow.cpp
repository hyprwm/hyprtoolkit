#include "ToolkitWindow.hpp"

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../core/AnimationManager.hpp"

using namespace Hyprtoolkit;

void IToolkitWindow::damage(const Hyprutils::Math::CRegion& rg) {
    auto newRg = rg.copy().scale(scale());

    if (m_damageRing.damage(newRg))
        scheduleFrame();
}

void IToolkitWindow::damageEntire() {
    m_damageRing.damageEntire();

    scheduleFrame();
}

void IToolkitWindow::scheduleFrame() {
    if (m_needsFrame)
        return;

    m_needsFrame = true;
    g_backend->addIdle([this, self = m_self] {
        if (!self)
            return;
        render();
    });
}

void IToolkitWindow::onPreRender() {
    g_animationManager->tick();

    for (const auto& e : m_needsReposition) {
        g_positioner->repositionNeeded(e.lock());
    }
    m_needsReposition.clear();
}

void IToolkitWindow::scheduleReposition(WP<IElement> e) {
    m_needsReposition.emplace_back(e);
    scheduleFrame();
}
