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

    g_animationManager->createAnimation(data.color(), m_impl->color, g_animationManager->m_animationTree.getConfig("fast"));
    m_impl->color->setUpdateCallback([this](auto) { impl->damageEntire(); });
    m_impl->color->setCallbackOnBegin([this](auto) { impl->damageEntire(); }, false);

    g_animationManager->createAnimation(data.borderColor(), m_impl->borderColor, g_animationManager->m_animationTree.getConfig("fast"));
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
            .color    = m_impl->borderColor->value(),
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

void CRectangleElement::replaceData(const SRectangleData& data) {
    m_impl->data         = data;
    *m_impl->color       = data.color();
    *m_impl->borderColor = data.borderColor();
    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CRectangleElement::recheckColor() {
    *m_impl->color       = m_impl->data.color();
    *m_impl->borderColor = m_impl->data.borderColor();
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
