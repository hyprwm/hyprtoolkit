#include "Element.hpp"

#include <hyprutils/math/Box.hpp>

#include "../helpers/Memory.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

IElement::IElement() {
    impl = UP<SElementInternalData>(new SElementInternalData());
}

void IElement::setPositionMode(ePositionMode mode) {
    impl->positionMode = mode;
}

void IElement::setAbsolutePosition(const Hyprutils::Math::Vector2D& offset) {
    impl->absoluteOffset = offset;
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
    impl->grow = grow;
}

void IElement::addChild(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (std::ranges::find(impl->children, child) != impl->children.end())
        return;

    child->impl->parent = impl->self.lock();
    child->impl->window = impl->window;
    child->impl->breadthfirst([w = impl->window.lock()](SP<IElement> e) { e->impl->setWindow(w); });
    impl->children.emplace_back(child);
}

void IElement::clearChildren() {
    for (auto& c : impl->children) {
        c->impl->parent.reset();
        c->impl->window.reset();
    }
    impl->children.clear();
}

bool IElement::acceptsMouseInput() {
    return false;
}

ePointerShape IElement::pointerShape() {
    return HT_POINTER_ARROW;
}

void IElement::setMargin(float thick) {
    impl->margin = thick;
}

void IElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    impl->setPosition(box);
}

void IElement::recheckColor() {
    ;
}

void SElementInternalData::setPosition(const CBox& box) {
    position = box;
    position.expand(-margin);
}

void SElementInternalData::bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn) {
    for (const auto& e : elements) {
        fn(e);
    }

    std::vector<SP<IElement>> els;
    for (const auto& e : elements) {
        for (const auto& c : e->impl->children) {
            els.emplace_back(c);
        }
    }

    if (!els.empty())
        bfHelper(els, fn);
}

void SElementInternalData::breadthfirst(const std::function<void(SP<IElement>)>& fn) {
    fn(self.lock());

    std::vector<SP<IElement>> els = children;

    bfHelper(els, fn);
}

void SElementInternalData::setWindow(SP<IToolkitWindow> w) {
    window = w;
    if (w)
        w->scheduleReposition(self);
}

void SElementInternalData::damageEntire() {
    if (!window)
        return;
    window->damage(position);
}
