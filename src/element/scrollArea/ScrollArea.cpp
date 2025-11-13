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
        if (m_impl->data.blockUserScroll)
            return;

        const bool SCROLLING_X = axis == Input::AXIS_AXIS_HORIZONTAL;

        if (SCROLLING_X && !m_impl->data.scrollX)
            return;
        if (!SCROLLING_X && !m_impl->data.scrollY)
            return;

        if (SCROLLING_X)
            m_impl->data.currentScroll.x += delta;
        else
            m_impl->data.currentScroll.y += delta;

        m_impl->clampMaxScroll();

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

    m_impl->clampMaxScroll();

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

Vector2D CScrollAreaElement::getCurrentScroll() {
    return m_impl->data.currentScroll;
}

void CScrollAreaElement::setScroll(const Hyprutils::Math::Vector2D& x) {
    m_impl->data.currentScroll = x;
    m_impl->clampMaxScroll();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

std::optional<Vector2D> CScrollAreaElement::preferredSize(const Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
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

bool CScrollAreaElement::positioningDependsOnChild() {
    return false;
}

void SScrollAreaImpl::clampMaxScroll() {
    // recheck limits
    if (self->impl->children.empty() || !self->impl->children.at(0)->impl->positionerData)
        return;

    Vector2D scrollMax = (self->impl->children.at(0)
                              ->preferredSize({
                                  data.scrollX ? 99999999999 : self->impl->position.w,
                                  data.scrollY ? 99999999999 : self->impl->position.h,
                              })
                              .value_or(Vector2D{99999999, 99999999}) -
                          self->impl->position.size())
                             .clamp({0, 0});

    data.currentScroll = data.currentScroll.clamp({}, scrollMax);
}
