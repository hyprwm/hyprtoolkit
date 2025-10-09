#include "ToolkitWindow.hpp"

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../core/AnimationManager.hpp"
#include "../element/Element.hpp"
#include "../Macros.hpp"

#include <algorithm>

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
    if (m_el)
        m_el->impl->toolkitWindowData->unlock();
}

void IToolkitWindow::damage(Hyprutils::Math::CRegion&& rg) {
    rg.scale(scale());

    if (m_damageRing.damage(std::move(rg)))
        scheduleFrame();
}

void IToolkitWindow::damageEntire() {
    m_damageRing.damageEntire();

    scheduleFrame();
}

void IToolkitWindow::scheduleFrame() {
    if (m_scheduledRender)
        return;

    m_scheduledRender = true;
    m_needsFrame      = true;
    g_backend->addIdle([this, self = m_self] {
        if (!self)
            return;
        m_scheduledRender = false;
        render();
    });
}

void IToolkitWindow::onPreRender() {
    g_animationManager->tick();

    // simplify repositions: step 1, expand ancestors
    for (auto& e : m_needsReposition) {
        if (!e)
            continue;
        while (e->impl->parent && e->impl->parent->positioningDependsOnChild()) {
            e = e->impl->parent;
        }
    }

    // step 2: many will be repeated. Only calculate those that don't have any parent above already
    // scheduled
    std::erase_if(m_needsReposition, [this](WP<IElement> e) {
        if (!e)
            return true;

        while (e->impl->parent) {
            if (std::ranges::find(m_needsReposition, e->impl->parent) != m_needsReposition.end())
                return true;

            e = e->impl->parent.lock();
        }

        return false;
    });

    // step 3: eliminate same-parent nodes
    // since repositionNeeded will reposition the parent's children,
    // we don't need to do it 200 times if the parent has 200 children and all need a reposition
    std::vector<WP<IElement>> parents;
    parents.reserve(m_needsReposition.size());
    std::erase_if(m_needsReposition, [&parents](WP<IElement> e) {
        if (std::ranges::contains(parents, e->impl->parent))
            return true;

        parents.emplace_back(e->impl->parent);
        return false;
    });

    for (const auto& e : m_needsReposition) {
        g_positioner->repositionNeeded(e.lock());
    }
    m_needsReposition.clear();

    if (m_needsReposition.empty())
        return;

    g_positioner->position(m_rootElement, {{}, pixelSize() / scale()});
}

void IToolkitWindow::scheduleReposition(WP<IElement> e) {
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
            // check for any parent being a clip
            auto parent = current->impl->parent;
            while (parent) {
                if (parent->impl->clipChildren && !parent->impl->position.containsPoint(coords))
                    return; // nope

                parent = parent->impl->parent;
            }
            el = current;
            if (current->alwaysGetMouseInput()) {
                initElementIfNeeded(el);
                alwaysHover.emplace_back(makeShared<SToolkitFocusLock>(el, coords - el->impl->position.pos()));
            }
        }
    });

    m_hoveredElements = alwaysHover;

    if ((el == (m_mainHoverElement ? m_mainHoverElement->m_el : WP<IElement>{})) || m_mouseIsDown /* Lock focus while mouse is down */)
        return;

    if (el) {
        initElementIfNeeded(el);
        m_mainHoverElement = makeShared<SToolkitFocusLock>(el, coords - el->impl->position.pos());
    } else
        m_mainHoverElement.reset();
    setCursor(m_mainHoverElement && m_mainHoverElement->m_el ? m_mainHoverElement->m_el->pointerShape() : HT_POINTER_ARROW);
}

void IToolkitWindow::mouseEnter(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);

    if (m_mainHoverElement && m_mainHoverElement->m_el)
        m_mainHoverElement->m_el->impl->m_externalEvents.mouseMove.emit(local - m_mainHoverElement->m_el->impl->position.pos());

    for (const auto& e : m_hoveredElements) {
        if (!e->m_el)
            continue;
        e->m_el->impl->m_externalEvents.mouseMove.emit(local - e->m_el->impl->position.pos());
    }
}

void IToolkitWindow::mouseMove(const Hyprutils::Math::Vector2D& local) {
    updateFocus(local);

    if (m_mainHoverElement && m_mainHoverElement->m_el)
        m_mainHoverElement->m_el->impl->m_externalEvents.mouseMove.emit(local - m_mainHoverElement->m_el->impl->position.pos());

    for (const auto& e : m_hoveredElements) {
        if (!e->m_el)
            continue;
        e->m_el->impl->m_externalEvents.mouseMove.emit(local - e->m_el->impl->position.pos());
    }
}

void IToolkitWindow::mouseButton(const Input::eMouseButton button, bool state) {
    m_mouseIsDown = state;

    if (state) {
        if (m_mainHoverElement && m_mainHoverElement->m_el && m_mainHoverElement->m_el->acceptsKeyboardInput() && m_keyboardFocus != m_mainHoverElement->m_el) {
            // enter this element
            if (m_keyboardFocus)
                m_keyboardFocus->impl->m_externalEvents.keyboardLeave.emit();
            m_keyboardFocus = m_mainHoverElement->m_el;
            m_keyboardFocus->impl->m_externalEvents.keyboardEnter.emit();
        } else if (m_keyboardFocus != m_mainHoverElement->m_el) {
            if (m_keyboardFocus)
                m_keyboardFocus->impl->m_externalEvents.keyboardLeave.emit();
            m_keyboardFocus.reset();
        }
    }

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

void IToolkitWindow::keyboardKey(const Input::SKeyboardKeyEvent& ev) {
    m_events.keyboardKey.emit(ev);

    if (!m_keyboardFocus)
        return;

    m_keyboardFocus->impl->m_externalEvents.key.emit(ev);
}

void IToolkitWindow::unfocusKeyboard() {
    if (!m_keyboardFocus)
        return;

    m_keyboardFocus->impl->m_externalEvents.keyboardLeave.emit();
    m_keyboardFocus.reset();
}

void IToolkitWindow::setKeyboardFocus(SP<IElement> e) {
    unfocusKeyboard();

    if (!e->acceptsKeyboardInput())
        return;

    m_keyboardFocus = e;
    e->impl->m_externalEvents.keyboardEnter.emit();
}

void IToolkitWindow::setIMTo(const Hyprutils::Math::CBox& box, const std::string& str, size_t cursor) {
    ;
}

void IToolkitWindow::resetIM() {
    ;
}

Hyprutils::Math::Vector2D IToolkitWindow::cursorPos() {
    return m_mousePos;
}
