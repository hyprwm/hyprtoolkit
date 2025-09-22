#include "ToolkitWindow.hpp"

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../core/AnimationManager.hpp"
#include "../element/Element.hpp"

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

void IToolkitWindow::updateFocus(const Hyprutils::Math::Vector2D& coords) {
    m_mousePos = coords;

    SP<IElement> el;
    m_rootElement->impl->breadthfirst([&el, coords](SP<IElement> current) {
        if (current->acceptsMouseInput() && current->impl->position.containsPoint(coords))
            el = current;
    });

    if (el == m_hoveredElement) {
        if (m_hoveredElement)
            m_hoveredElement->impl->m_externalEvents.mouseMove.emit(coords - m_hoveredElement->impl->position.pos());
        return;
    }

    if (m_hoveredElement)
        m_hoveredElement->impl->m_externalEvents.mouseLeave.emit();
    m_hoveredElement = el;
    if (m_hoveredElement)
        m_hoveredElement->impl->m_externalEvents.mouseEnter.emit(coords - m_hoveredElement->impl->position.pos());

    setCursor(m_hoveredElement ? m_hoveredElement->pointerShape() : HT_POINTER_ARROW);
}

void IToolkitWindow::mouseEnter(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);
}

void IToolkitWindow::mouseMove(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);
}

void IToolkitWindow::mouseButton(const Input::eMouseButton button, bool state) {
    if (!m_hoveredElement)
        return;

    m_hoveredElement->impl->m_externalEvents.mouseButton.emit(button, state);
}

void IToolkitWindow::mouseLeave() {
    if (!m_hoveredElement)
        return;

    m_hoveredElement->impl->m_externalEvents.mouseLeave.emit();
    m_hoveredElement.reset();
}
