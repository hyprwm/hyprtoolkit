#include "Rectangle.hpp"

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;

SP<CRectangleElement> CRectangleElement::create(const SRectangleData& data) {
    auto p          = SP<CRectangleElement>(new CRectangleElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CRectangleElement::CRectangleElement(const SRectangleData& data) : IElement(), m_impl(makeUnique<SRectangleImpl>()) {
    m_impl->data = data;

    m_impl->colorAnimationConfig = g_animationManager->m_animationTree.getConfig("fast");
    g_animationManager->createAnimation(data.color(), m_impl->color, m_impl->colorAnimationConfig);
    m_impl->color->setUpdateCallback([this](auto) {
        if (impl->window)
            impl->window->m_opaqueRegionDirty = true;
        impl->damageEntire();
    });
    m_impl->color->setCallbackOnBegin(
        [this](auto) {
            if (impl->window)
                impl->window->m_opaqueRegionDirty = true;
            impl->damageEntire();
        },
        false);

    m_impl->borderAnimationConfig = g_animationManager->m_animationTree.getConfig("fast");
    g_animationManager->createAnimation(data.borderColor(), m_impl->borderColor, m_impl->borderAnimationConfig);
    m_impl->borderColor->setUpdateCallback([this](auto) {
        if (m_impl->data.borderThickness)
            impl->damageEntire();
    });
    m_impl->borderColor->setCallbackOnBegin(
        [this](auto) {
            if (m_impl->data.borderThickness)
                impl->damageEntire();
        },
        false);
}

void CRectangleElement::paint() {
    g_renderer->renderRectangle({
        .box      = impl->position,
        .color    = m_impl->color->value(),
        .rounding = m_impl->data.rounding,
    });

    if (m_impl->data.borderThickness > 0) {
        g_renderer->renderBorder({
            .box      = impl->position,
            .gradient = m_impl->borderColor->value(),
            .rounding = m_impl->data.rounding,
            .thick    = m_impl->data.borderThickness,
        });
    }
}

void CRectangleElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CRectangleBuilder> CRectangleElement::rebuild() {
    auto p       = SP<CRectangleBuilder>(new CRectangleBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SRectangleData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CRectangleElement::animateColor(const SAnimation& animation) {
    if (std::holds_alternative<SNoAnimation>(animation))
        m_impl->color->setValueAndWarp(m_impl->color->goal());
    m_impl->colorAnimationConfig = g_animationManager->configFor(animation);
}

void CRectangleElement::animateBorderColor(const SAnimation& animation) {
    if (std::holds_alternative<SNoAnimation>(animation))
        m_impl->borderColor->setValueAndWarp(m_impl->borderColor->goal());
    m_impl->borderAnimationConfig = g_animationManager->configFor(animation);
}

void CRectangleElement::replaceData(const SRectangleData& data) {
    const auto COLOR        = data.color();
    const auto BORDER_COLOR = data.borderColor();
    m_impl->data            = data;
    if (m_impl->color->isBeingAnimated() && (m_impl->color->getConfig() != m_impl->colorAnimationConfig || (m_impl->color->isSpringCurve() && m_impl->color->goal() != COLOR)))
        m_impl->color->setValueAndWarp(m_impl->color->value());
    if (m_impl->borderColor->isBeingAnimated() &&
        (m_impl->borderColor->getConfig() != m_impl->borderAnimationConfig || (m_impl->borderColor->isSpringCurve() && m_impl->borderColor->goal() != BORDER_COLOR)))
        m_impl->borderColor->setValueAndWarp(m_impl->borderColor->value());
    m_impl->color->setConfig(m_impl->colorAnimationConfig);
    m_impl->borderColor->setConfig(m_impl->borderAnimationConfig);
    if (m_impl->color->enabled())
        *m_impl->color = COLOR;
    else
        m_impl->color->setValueAndWarp(COLOR);
    if (m_impl->borderColor->enabled())
        *m_impl->borderColor = BORDER_COLOR;
    else
        m_impl->borderColor->setValueAndWarp(BORDER_COLOR);
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CRectangleElement::recheckColor() {
    const auto COLOR        = m_impl->data.color();
    const auto BORDER_COLOR = m_impl->data.borderColor();
    if (m_impl->color->isBeingAnimated() && (m_impl->color->getConfig() != m_impl->colorAnimationConfig || (m_impl->color->isSpringCurve() && m_impl->color->goal() != COLOR)))
        m_impl->color->setValueAndWarp(m_impl->color->value());
    if (m_impl->borderColor->isBeingAnimated() &&
        (m_impl->borderColor->getConfig() != m_impl->borderAnimationConfig || (m_impl->borderColor->isSpringCurve() && m_impl->borderColor->goal() != BORDER_COLOR)))
        m_impl->borderColor->setValueAndWarp(m_impl->borderColor->value());
    m_impl->color->setConfig(m_impl->colorAnimationConfig);
    m_impl->borderColor->setConfig(m_impl->borderAnimationConfig);
    if (m_impl->color->enabled())
        *m_impl->color = COLOR;
    else
        m_impl->color->setValueAndWarp(COLOR);
    if (m_impl->borderColor->enabled())
        *m_impl->borderColor = BORDER_COLOR;
    else
        m_impl->borderColor->setValueAndWarp(BORDER_COLOR);
}

Hyprutils::Math::Vector2D CRectangleElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CRectangleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CRectangleElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return Vector2D{0, 0};
}

std::optional<Vector2D> CRectangleElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;
    return std::nullopt;
}

bool CRectangleElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}

CBox CRectangleElement::opaqueBox() {
    if (m_impl->color->value().a != 1.F || impl->effectiveOpacity() != 1.F || impl->hasActiveGeometry())
        return {};

    CBox opaque = impl->position;
    opaque.x    = 0;
    opaque.y    = 0;
    opaque.expand(-(m_impl->data.rounding + m_impl->data.borderThickness));
    return opaque;
}
