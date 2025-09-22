#include <hyprtoolkit/element/Null.hpp>

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CNullElement> CNullElement::create(const SNullData& data) {
    auto p        = SP<CNullElement>(new CNullElement(data));
    p->impl->self = p;
    return p;
}

CNullElement::CNullElement(const SNullData& data) : IElement(), m_data(data) {
    ;
}

void CNullElement::paint() {
    ;
}

void CNullElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

Hyprutils::Math::Vector2D CNullElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CNullElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_data.size.calculate(parent);
}

std::optional<Vector2D> CNullElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CNullElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}
