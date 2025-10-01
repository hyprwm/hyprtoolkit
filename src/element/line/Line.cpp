#include "Line.hpp"

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;

SP<CLineElement> CLineElement::create(const SLineData& data) {
    auto p          = SP<CLineElement>(new CLineElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CLineElement::CLineElement(const SLineData& data) : IElement(), m_impl(makeUnique<SLineImpl>()) {
    m_impl->data = data;
}

void CLineElement::paint() {
    g_renderer->renderLine({
        .box    = impl->position,
        .points = m_impl->data.points,
        .color  = m_impl->data.color(),
        .thick  = m_impl->data.thick,
    });
}

void CLineElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CLineBuilder> CLineElement::rebuild() {
    auto p       = SP<CLineBuilder>(new CLineBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SLineData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CLineElement::replaceData(const SLineData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CLineElement::recheckColor() {
    ;
}

Hyprutils::Math::Vector2D CLineElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CLineElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CLineElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CLineElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}

bool CLineElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
