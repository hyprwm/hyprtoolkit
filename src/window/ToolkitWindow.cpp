#include "ToolkitWindow.hpp"

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../core/AnimationManager.hpp"
#include "../element/Element.hpp"
#include "../Macros.hpp"

using namespace Hyprtoolkit;

struct Hyprtoolkit::SToolkitWindowData {
    void         lock(const Hyprutils::Math::Vector2D& coord = {});
    void         unlock();

    size_t       cursorFocusLocks = 0;
    WP<IElement> self;
};

void SToolkitWindowData::lock(const Hyprutils::Math::Vector2D& coord) {
    cursorFocusLocks++;

    if (cursorFocusLocks == 1)
        self->impl->m_externalEvents.mouseEnter.emit(coord);
}

void SToolkitWindowData::unlock() {
    RASSERT(cursorFocusLocks, "SToolkitWindowData::unlock() on no locks");
    cursorFocusLocks--;

    if (!cursorFocusLocks && !self->impl->window.expired())
        self->impl->m_externalEvents.mouseLeave.emit();
}

SToolkitFocusLock::SToolkitFocusLock(SP<IElement> e, const Hyprutils::Math::Vector2D& coord) : m_el(e) {
    m_el->impl->toolkitWindowData->lock(coord);
}

SToolkitFocusLock::~SToolkitFocusLock() {
    m_el->impl->toolkitWindowData->unlock();
}

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
    damageEntire();
    m_needsReposition.emplace_back(e);
    scheduleFrame();
}

void IToolkitWindow::initElementIfNeeded(SP<IElement> e) {
    if (e->impl->toolkitWindowData)
        return;

    e->impl->toolkitWindowData       = makeUnique<SToolkitWindowData>();
    e->impl->toolkitWindowData->self = e;
}

void IToolkitWindow::updateFocus(const Hyprutils::Math::Vector2D& coords) {
    m_mousePos = coords;

    SP<IElement>                       el;
    std::vector<SP<SToolkitFocusLock>> alwaysHover;
    m_rootElement->impl->breadthfirst([this, &el, &alwaysHover, coords](SP<IElement> current) {
        if (current->acceptsMouseInput() && current->impl->position.containsPoint(coords)) {
            el = current;
            if (current->alwaysGetMouseInput()) {
                initElementIfNeeded(el);
                alwaysHover.emplace_back(makeShared<SToolkitFocusLock>(el, coords - el->impl->position.pos()));
            }
        }
    });

    m_hoveredElements = alwaysHover;

    for (const auto& e : m_hoveredElements) {
        if (!e->m_el)
            continue;
        e->m_el->impl->m_externalEvents.mouseMove.emit(coords - e->m_el->impl->position.pos());
    }

    if ((el == (m_mainHoverElement ? m_mainHoverElement->m_el : WP<IElement>{})) || m_mouseIsDown /* Lock focus while mouse is down */) {
        if (el)
            el->impl->m_externalEvents.mouseMove.emit(coords - el->impl->position.pos());
        return;
    }

    if (el) {
        initElementIfNeeded(el);
        m_mainHoverElement = makeShared<SToolkitFocusLock>(el, coords - el->impl->position.pos());
    } else
        m_mainHoverElement.reset();
    setCursor(m_mainHoverElement && m_mainHoverElement->m_el ? m_mainHoverElement->m_el->pointerShape() : HT_POINTER_ARROW);
}

void IToolkitWindow::mouseEnter(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);
}

void IToolkitWindow::mouseMove(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);
}

void IToolkitWindow::mouseButton(const Input::eMouseButton button, bool state) {
    m_mouseIsDown = state;

    if (m_mainHoverElement && m_mainHoverElement->m_el)
        m_mainHoverElement->m_el->impl->m_externalEvents.mouseButton.emit(button, state);

    for (const auto& e : m_hoveredElements) {
        if (!e->m_el)
            continue;
        e->m_el->impl->m_externalEvents.mouseButton.emit(button, state);
    }
}

void IToolkitWindow::mouseAxis(const Input::eAxisAxis axis, float delta) {

    if (m_mainHoverElement && m_mainHoverElement->m_el)
        m_mainHoverElement->m_el->impl->m_externalEvents.mouseAxis.emit(axis, delta);

    for (const auto& e : m_hoveredElements) {
        if (!e->m_el)
            continue;
        e->m_el->impl->m_externalEvents.mouseAxis.emit(axis, delta);
    }
}

void IToolkitWindow::mouseLeave() {
    m_mainHoverElement.reset();
    m_hoveredElements.clear();
}
