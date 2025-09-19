#include <hyprtoolkit/element/Rectangle.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"

using namespace Hyprtoolkit;

CRectangleElement::CRectangleElement(const SRectangleData& data) : m_data(data) {
    ;
}

void CRectangleElement::paint() {
    g_renderer->renderRectangle({
        .box      = m_position,
        .color    = m_data.color,
        .a        = m_data.a,
        .rounding = m_data.rounding,
    });
}

void CRectangleElement::reposition(const Hyprutils::Math::CBox& box) {
    m_position = box;

    const auto C = m_children;

    for (const auto& c : C) {
        g_positioner->position(c, m_position);
    }
}

Hyprutils::Math::Vector2D CRectangleElement::size() {
    return m_position.size();
}

std::optional<Vector2D> CRectangleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CRectangleElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return Vector2D{0, 0};
}
