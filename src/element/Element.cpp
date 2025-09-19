#include <hyprtoolkit/element/Element.hpp>

using namespace Hyprtoolkit;

void IElement::setPositionMode(ePositionMode mode) {
    m_positionMode = mode;
}

void IElement::setAbsolutePosition(const Hyprutils::Math::Vector2D& offset) {
    m_absoluteOffset = offset;
}

std::optional<Hyprutils::Math::Vector2D> IElement::preferredSize() {
    return std::nullopt;
}

std::optional<Hyprutils::Math::Vector2D> IElement::minimumSize() {
    return std::nullopt;
}

std::optional<Hyprutils::Math::Vector2D> IElement::maximumSize() {
    return std::nullopt;
}

void IElement::setGrow(bool grow) {
    m_grow = grow;
}
