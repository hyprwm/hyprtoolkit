#include "Element.hpp"

#include <hyprutils/math/Box.hpp>
#include <hyprtoolkit/types/SizeType.hpp>

#include "../helpers/Memory.hpp"
#include "../window/ToolkitWindow.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

IElement::IElement() {
    impl = UP<SElementInternalData>(new SElementInternalData());
}

IElement::~IElement() {
    impl.reset();
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
    impl->growH = grow;
    impl->growV = grow;
}

void IElement::setGrow(bool growH, bool growV) {
    impl->growH = growH;
    impl->growV = growV;
}

void IElement::addChild(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (std::ranges::find(impl->children, child) != impl->children.end())
        return;

    child->impl->parent = impl->self.lock();
    child->impl->window = impl->window;
    child->impl->breadthfirst([w = impl->window.lock()](SP<IElement> e) { e->impl->setWindow(w); });
    impl->children.emplace_back(child);

    if (impl->window)
        impl->window->scheduleReposition(child);
}

void IElement::removeChild(Hyprutils::Memory::CSharedPointer<IElement> child) {
    if (std::ranges::find(impl->children, child) == impl->children.end())
        return;

    std::erase(impl->children, child);

    child->impl->parent.reset();
    child->impl->window.reset();
    child->impl->breadthfirst([](SP<IElement> e) { e->impl->setWindow(nullptr); });

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
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

bool IElement::alwaysGetMouseInput() {
    return false;
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

bool IElement::acceptsKeyboardInput() {
    return false;
}

void IElement::imCommitNewText(const std::string& text) {
    ;
}

void IElement::imApplyText() {
    ;
}

void SElementInternalData::setPosition(const CBox& box) {
    position = box;
    if (margin > 0)
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
    window->damage(position.copy().expand(2));
}

void SElementInternalData::setFailedPositioning(bool set) {
    breadthfirst([set](SP<IElement> e) { e->impl->failedPositioning = set; });
}

Vector2D SElementInternalData::maxChildSize(const Vector2D& parent) {
    Vector2D max;
    for (const auto& e : children) {
        auto size = e->preferredSize(parent);
        if (!size)
            size = e->minimumSize(parent);

        if (!size)
            continue;

        max.x = std::max(max.x, size->x);
        max.y = std::max(max.y, size->y);
    }
    return max + Vector2D{margin * 2, margin * 2};
}

Vector2D SElementInternalData::getPreferredSizeGeneric(const CDynamicSize& size, const Vector2D& parent) {
    auto s = size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    auto max = maxChildSize(parent - Vector2D{margin * 2, margin * 2});
    if (s.x == -1)
        s.x = max.x;
    if (s.y == -1)
        s.y = max.y;
    return s;
}
