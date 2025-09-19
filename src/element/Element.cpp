#include <hyprtoolkit/element/Element.hpp>

using namespace Hyprtoolkit;

void IElement::setPositionMode(ePositionMode mode) {
    m_positionMode = mode;
}

void IElement::setAbsolutePosition(const Hyprutils::Math::Vector2D& offset) {
    m_absoluteOffset = offset;
}
