#include <hyprtoolkit/element/Texture.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"

#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CTextureElement> CTextureElement::create(const STextureData& data) {
    auto p = SP<CTextureElement>(new CTextureElement(data));
    p->m_self = p;
    return p;
}

CTextureElement::CTextureElement(const STextureData& data) : IElement(), m_data(data) {
    ;
}

void CTextureElement::paint() {
    ;
}

void CTextureElement::reposition(const Hyprutils::Math::CBox& box) {
    m_position = box;

    const auto C = m_elementData->children;

    for (const auto& c : C) {
        g_positioner->position(c, m_position);
    }
}

Hyprutils::Math::Vector2D CTextureElement::size() {
    return m_position.size();
}

std::optional<Vector2D> CTextureElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CTextureElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return Vector2D{0, 0};
}
