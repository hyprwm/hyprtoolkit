#include <hyprtoolkit/element/Rectangle.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CRectangleElement> CRectangleElement::create(const SRectangleData& data) {
    auto p = SP<CRectangleElement>(new CRectangleElement(data));
    p->impl->self = p;
    return p;
}

CRectangleElement::CRectangleElement(const SRectangleData& data) : IElement(), m_data(data) {
    ;
}

void CRectangleElement::paint() {
    g_renderer->renderRectangle({
        .box      = impl->position,
        .color    = m_data.color,
        .a        = m_data.a,
        .rounding = m_data.rounding,
    });
}

void CRectangleElement::reposition(const Hyprutils::Math::CBox& box) {
    impl->position = box;

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

Hyprutils::Math::Vector2D CRectangleElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CRectangleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CRectangleElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return Vector2D{0, 0};
}
