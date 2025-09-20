#include "Element.hpp"

#include "../helpers/Memory.hpp"

using namespace Hyprtoolkit;

IElement::IElement() {
    m_elementData = UP<SElementInternalData>(new SElementInternalData());
}

void IElement::setPositionMode(ePositionMode mode) {
    m_elementData->positionMode = mode;
}

void IElement::setAbsolutePosition(const Hyprutils::Math::Vector2D& offset) {
    m_elementData->absoluteOffset = offset;
}

std::optional<Hyprutils::Math::Vector2D> IElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return std::nullopt;
}

std::optional<Hyprutils::Math::Vector2D> IElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return std::nullopt;
}

std::optional<Hyprutils::Math::Vector2D> IElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return std::nullopt;
}

void IElement::setGrow(bool grow) {
    m_elementData->grow = grow;
}

void IElement::addChild(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (std::ranges::find(m_elementData->children, child) != m_elementData->children.end())
        return;

    m_elementData->children.emplace_back(child);
}

void IElement::clearChildren() {
    m_elementData->children.clear();
}
