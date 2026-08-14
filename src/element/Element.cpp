#include "Element.hpp"

#include <hyprutils/math/Box.hpp>
#include <hyprtoolkit/types/SizeType.hpp>

#include "../helpers/Memory.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../layout/Positioner.hpp"
#include "../core/AnimationManager.hpp"

#include <algorithm>
#include <cmath>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

IElement::IElement() {
    impl = UP<SElementInternalData>(new SElementInternalData());
    impl->m_externalEvents.mouseEnter.listenStatic([this](Vector2D local) {
        if (!impl->userRequestedMouseInput)
            return;

        if (impl->userFns.mouseEnter)
            impl->userFns.mouseEnter(local);
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        if (!impl->userRequestedMouseInput)
            return;

        if (impl->userFns.mouseLeave)
            impl->userFns.mouseLeave();
    });
    impl->m_externalEvents.mouseMove.listenStatic([this](Vector2D local) {
        if (!impl->userRequestedMouseInput)
            return;

        if (impl->userFns.mouseMove)
            impl->userFns.mouseMove(local);
    });
    impl->m_externalEvents.mouseButton.listenStatic([this](Input::eMouseButton button, bool down) {
        if (!impl->userRequestedMouseInput)
            return;

        if (impl->userFns.mouseButton)
            impl->userFns.mouseButton(button, down);
    });
    impl->m_externalEvents.mouseAxis.listenStatic([this](Input::eAxisAxis axis, bool down) {
        if (!impl->userRequestedMouseInput)
            return;

        if (impl->userFns.mouseAxis)
            impl->userFns.mouseAxis(axis, down);
    });
}

IElement::~IElement() {
    impl.reset();
}

void IElement::setPositionMode(ePositionMode mode) {
    impl->positionMode = mode;
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void IElement::setPositionFlag(ePositionFlag flag, bool set) {
    if (set)
        impl->positionFlags |= flag;
    else
        impl->positionFlags &= ~flag;
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void IElement::setAbsolutePosition(const Hyprutils::Math::Vector2D& offset) {
    impl->absoluteOffset = offset;
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void IElement::setTooltip(std::string&& x) {
    impl->tooltip    = std::move(x);
    impl->hasTooltip = !impl->tooltip.empty();
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

static void installOpacityAnimation(IElement* element, const SAnimation& animation) {
    auto& data                   = element->impl;
    data->opacityAnimated        = true;
    data->opacityAnimationConfig = g_animationManager->configFor(animation);
    if (!data->animatedOpacity) {
        g_animationManager->createAnimation(data->opacity, data->animatedOpacity, data->opacityAnimationConfig);
        data->animatedOpacity->setCallbackOnBegin([element](auto) { element->impl->damagePresentation(); }, false);
        data->animatedOpacity->setUpdateCallback([element](auto) { element->impl->damagePresentation(); });
    }
}

static CBox geometryBoxFor(const SElementInternalData& data) {
    auto geometry = data.position;
    if (!data.parent || data.parent->impl->position.empty())
        return geometry;

    const auto& PARENT = data.parent->impl->position;
    geometry.x         = (geometry.x - PARENT.x) / PARENT.w;
    geometry.y         = (geometry.y - PARENT.y) / PARENT.h;
    geometry.w /= PARENT.w;
    geometry.h /= PARENT.h;
    return geometry;
}

void IElement::animateOpacity(const SAnimation& animation) {
    if (std::holds_alternative<SNoAnimation>(animation)) {
        if (impl->animatedOpacity) {
            impl->opacity = std::clamp(impl->animatedOpacity->goal(), 0.F, 1.F);
            impl->animatedOpacity->resetAllCallbacks();
            impl->animatedOpacity.reset();
        }
        impl->opacityAnimated = false;
        impl->opacityAnimationConfig.reset();
        impl->damagePresentation();
        return;
    }

    installOpacityAnimation(this, animation);
}

static void installGeometryAnimation(IElement* element, const SAnimation& animation) {
    auto& data                    = element->impl;
    data->geometryAnimated        = true;
    data->geometryAnimationConfig = g_animationManager->configFor(animation);
    if (!data->animatedGeometry) {
        const auto geometry = geometryBoxFor(*data);
        g_animationManager->createAnimation(geometry, data->animatedGeometry, data->geometryAnimationConfig);
        data->lastPresentationBounds = data->presentationSubtreeBox();
        data->animatedGeometry->setCallbackOnBegin([element](auto) { element->impl->damagePresentation(); }, false);
        data->animatedGeometry->setUpdateCallback([element](auto) { element->impl->damagePresentation(); });
    }
}

void IElement::animateGeometry(const SAnimation& animation) {
    if (std::holds_alternative<SNoAnimation>(animation)) {
        if (impl->animatedGeometry) {
            impl->animatedGeometry->resetAllCallbacks();
            impl->animatedGeometry.reset();
        }
        impl->geometryAnimated = false;
        impl->geometryAnimationConfig.reset();
        impl->damagePresentation();
        return;
    }

    installGeometryAnimation(this, animation);
}

void IElement::setOpacity(float opacity) {
    opacity = std::clamp(opacity, 0.F, 1.F);
    if (impl->opacityAnimated && impl->animatedOpacity) {
        if (impl->animatedOpacity->getConfig() != impl->opacityAnimationConfig && impl->animatedOpacity->isBeingAnimated())
            impl->animatedOpacity->setValueAndWarp(impl->animatedOpacity->value());
        impl->animatedOpacity->setConfig(impl->opacityAnimationConfig);
        *impl->animatedOpacity = opacity;
        return;
    }

    if (impl->opacity == opacity)
        return;

    impl->opacity = opacity;
    impl->damagePresentation();
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
        c->impl->breadthfirst([](SP<IElement> element) { element->impl->setWindow(nullptr); });
    }
    impl->children.clear();
}

bool IElement::acceptsMouseInput() {
    return impl->userRequestedMouseInput || impl->hasTooltip;
}

ePointerShape IElement::pointerShape() {
    return HT_POINTER_ARROW;
}

std::function<ePointerShape()> IElement::pointerShapeFn() {
    return nullptr;
}

bool IElement::alwaysGetMouseInput() {
    return false;
}

void IElement::setMargin(float thick) {
    impl->margin = thick;
}

void IElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    impl->setPosition(box);

    if (impl->userFns.repositioned)
        impl->userFns.repositioned();
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

void IElement::setReceivesMouse(bool x) {
    impl->userRequestedMouseInput = true;
}

void IElement::setMouseEnter(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn) {
    impl->userFns.mouseEnter = std::move(fn);
}

void IElement::setMouseLeave(std::function<void()>&& fn) {
    impl->userFns.mouseLeave = std::move(fn);
}

void IElement::setMouseMove(std::function<void(const Hyprutils::Math::Vector2D&)>&& fn) {
    impl->userFns.mouseMove = std::move(fn);
}

void IElement::setMouseButton(std::function<void(Input::eMouseButton, bool)>&& fn) {
    impl->userFns.mouseButton = std::move(fn);
}

void IElement::setMouseAxis(std::function<void(Input::eAxisAxis, float)>&& fn) {
    impl->userFns.mouseAxis = std::move(fn);
}

void IElement::setRepositioned(std::function<void()>&& fn) {
    impl->userFns.repositioned = std::move(fn);
}

void IElement::setGrouped(bool grouped) {
    impl->grouped = grouped;
}

Vector2D IElement::posFromParent() {
    if (!impl->parent)
        return impl->position.pos();
    return impl->position.pos() - impl->parent->impl->position.pos();
}

bool IElement::positioningDependsOnChild() {
    return false;
}

CBox IElement::opaqueBox() {
    return {};
}

void IElement::forceReposition() {
    g_positioner->repositionNeeded(impl->self.lock(), true);
}

void SElementInternalData::setPosition(const CBox& box) {
    const auto PREVIOUS = position;
    position            = box;
    if (margin > 0)
        position.expand(-margin);

    if (!geometryAnimated || !animatedGeometry)
        return;

    const auto geometry = geometryBoxFor(*this);

    if (!hasBeenPresented) {
        animatedGeometry->setConfig(geometryAnimationConfig);
        animatedGeometry->setValueAndWarp(geometry);
        lastPresentationBounds = presentationSubtreeBox();
        return;
    }

    if (PREVIOUS.empty() || animatedGeometry->value().empty()) {
        animatedGeometry->setValueAndWarp(geometry);
        lastPresentationBounds = presentationSubtreeBox();
        return;
    }

    if (animatedGeometry->goal() == geometry)
        return;

    const bool CONFIG_CHANGED = animatedGeometry->getConfig() != geometryAnimationConfig;
    if (animatedGeometry->isBeingAnimated() && (CONFIG_CHANGED || animatedGeometry->isSpringCurve()))
        animatedGeometry->setValueAndWarp(animatedGeometry->value());
    animatedGeometry->setConfig(geometryAnimationConfig);
    *animatedGeometry = geometry;
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
    window           = w;
    hasBeenPresented = false;
    if (w)
        w->scheduleReposition(self);
}

void SElementInternalData::damageEntire() {
    if (!window)
        return;
    window->damage(position.copy().expand(2));
}

static CBox applyBoxTransform(CBox box, const CBox& from, const CBox& to) {
    if (from.empty() || to.empty())
        return box;

    const Vector2D SCALE = {
        std::max(0.001, to.w / from.w),
        std::max(0.001, to.h / from.h),
    };

    box.x = to.x + ((box.x - from.x) * SCALE.x);
    box.y = to.y + ((box.y - from.y) * SCALE.y);
    box.w *= SCALE.x;
    box.h *= SCALE.y;
    return box;
}

CBox SElementInternalData::presentationBox(const CBox& box) const {
    auto transformed = box;
    auto current     = self;

    while (current) {
        const auto& DATA = current->impl;
        if (DATA->geometryAnimated && DATA->animatedGeometry) {
            auto geometry = DATA->animatedGeometry->value();
            if (DATA->parent && !DATA->parent->impl->position.empty()) {
                const auto& PARENT = DATA->parent->impl->position;
                geometry.x         = PARENT.x + (geometry.x * PARENT.w);
                geometry.y         = PARENT.y + (geometry.y * PARENT.h);
                geometry.w *= PARENT.w;
                geometry.h *= PARENT.h;
            }
            transformed = applyBoxTransform(transformed, DATA->position, geometry);
        }
        current = DATA->parent;
    }

    return transformed;
}

CBox SElementInternalData::presentationSubtreeBox() const {
    CBox bounds;
    bool initialized = false;

    const_cast<SElementInternalData*>(this)->breadthfirst([&](SP<IElement> element) {
        if (!element)
            return;

        const auto BOX = element->impl->presentationBox(element->impl->position);
        if (BOX.empty())
            return;

        if (!initialized) {
            bounds      = BOX;
            initialized = true;
            return;
        }

        const auto RIGHT  = std::max(bounds.x + bounds.w, BOX.x + BOX.w);
        const auto BOTTOM = std::max(bounds.y + bounds.h, BOX.y + BOX.h);
        bounds.x          = std::min(bounds.x, BOX.x);
        bounds.y          = std::min(bounds.y, BOX.y);
        bounds.w          = RIGHT - bounds.x;
        bounds.h          = BOTTOM - bounds.y;
    });

    return bounds;
}

float SElementInternalData::effectiveOpacity() const {
    float opacityValue = animatedOpacity ? animatedOpacity->value() : opacity;
    auto  current      = parent;

    while (current) {
        opacityValue *= current->impl->animatedOpacity ? current->impl->animatedOpacity->value() : current->impl->opacity;
        current = current->impl->parent;
    }

    return std::clamp(opacityValue, 0.F, 1.F);
}

bool SElementInternalData::hasActiveGeometry() const {
    auto current = self;
    while (current) {
        if (current->impl->animatedGeometry && current->impl->animatedGeometry->isBeingAnimated())
            return true;
        current = current->impl->parent;
    }
    return false;
}

void SElementInternalData::damagePresentation() {
    if (!window)
        return;

    window->m_opaqueRegionDirty = true;
    const auto CURRENT          = presentationSubtreeBox();
    if (!lastPresentationBounds.empty())
        window->damage(lastPresentationBounds.copy().expand(2));
    if (!CURRENT.empty())
        window->damage(CURRENT.copy().expand(2));
    lastPresentationBounds = CURRENT;
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
