#include "Rectangle.hpp"

#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

using namespace Hyprtoolkit;

SP<CRectangleElement> CRectangleElement::create(const SRectangleData& data) {
    auto p        = SP<CRectangleElement>(new CRectangleElement(data));
    p->impl->self = p;
    return p;
}

CRectangleElement::CRectangleElement(const SRectangleData& data) : IElement(), m_impl(makeUnique<SRectangleImpl>()) {
    m_impl->data = data;

    g_animationManager->createAnimation(CHyprColor{data.color, data.a}, m_impl->color, g_animationManager->m_animationTree.getConfig("fast"));
    m_impl->color->setUpdateCallback([this](auto) { impl->damageEntire(); });
    m_impl->color->setCallbackOnBegin([this](auto) { impl->damageEntire(); }, false);
}

void CRectangleElement::paint() {
    const auto c = m_impl->color->value();
    g_renderer->renderRectangle({
        .box      = impl->position,
        .color    = c.asRGB(),
        .a        = sc<float>(c.a),
        .rounding = m_impl->data.rounding,
    });
}

void CRectangleElement::reposition(const Hyprutils::Math::CBox& box) {
    impl->position = box;

    const auto C = impl->children;

    for (const auto& c : C) {
        g_positioner->position(c, impl->position);
    }
}

SRectangleData CRectangleElement::dataCopy() {
    return m_impl->data;
}

void CRectangleElement::replaceData(const SRectangleData& data) {
    m_impl->data   = data;
    *m_impl->color = CHyprColor{data.color, data.a};
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

Hyprutils::Math::Vector2D CRectangleElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CRectangleElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
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
