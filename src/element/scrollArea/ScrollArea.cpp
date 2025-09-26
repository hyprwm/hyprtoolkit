#include "ScrollArea.hpp"
#include <cmath>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"

#include "../Element.hpp"

using namespace Hyprtoolkit;

SP<CScrollAreaElement> CScrollAreaElement::create(const SScrollAreaData& data) {
    auto p          = SP<CScrollAreaElement>(new CScrollAreaElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CScrollAreaElement::CScrollAreaElement(const SScrollAreaData& data) : IElement(), m_impl(makeUnique<SScrollAreaImpl>()) {
    m_impl->data       = data;
    impl->clipChildren = true;

    m_impl->listeners.axis = impl->m_externalEvents.mouseAxis.listen([this](Input::eAxisAxis axis, float delta) {
        if (axis == Input::AXIS_AXIS_HORIZONTAL)
            m_impl->data.currentScroll.x += delta;
        else
            m_impl->data.currentScroll.y += delta;

        if (!impl->children.empty()) {
            const auto SCROLL_MAX =
                impl->children.at(0)->preferredSize(impl->children.at(0)->impl->positionerData->baseBox.size()).value_or({9999999, 9999999}) - impl->position.size();
            if (SCROLL_MAX.x < 0 || SCROLL_MAX.y < 0) // can't scroll: content is smaller
                m_impl->data.currentScroll = {};
            else
                m_impl->data.currentScroll = m_impl->data.currentScroll.clamp({0, 0}, SCROLL_MAX);
        } else
            m_impl->data.currentScroll = m_impl->data.currentScroll.clamp({0, 0});

        if (impl->window)
            impl->window->scheduleReposition(impl->self);
    });
}

void CScrollAreaElement::paint() {
    ; // no-op
}

void CScrollAreaElement::replaceData(const SScrollAreaData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CScrollAreaElement::reposition(const Hyprutils::Math::CBox& sbox, const Vector2D& maxSize) {
    IElement::reposition(sbox);

    g_positioner->positionChildren(impl->self.lock(),
                                   {
                                       .offset = -m_impl->data.currentScroll.round(),
                                       .growX  = m_impl->data.scrollX,
                                       .growY  = m_impl->data.scrollY,
                                   });
}

Vector2D CScrollAreaElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CScrollAreaElement::preferredSize(const Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CScrollAreaElement::minimumSize(const Vector2D& parent) {
    return Vector2D{0, 0};
}

bool CScrollAreaElement::acceptsMouseInput() {
    return true;
}

ePointerShape CScrollAreaElement::pointerShape() {
    return HT_POINTER_ARROW;
}

bool CScrollAreaElement::alwaysGetMouseInput() {
    return true;
}
