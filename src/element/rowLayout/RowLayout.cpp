#include "RowLayout.hpp"
#include <cmath>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"

#include "../Element.hpp"
#include "../LinearLayout.hpp"
#include "../text/Text.hpp"

using namespace Hyprtoolkit;

static Vector2D childSizeForConstraint(const SP<IElement>& child, const Vector2D& constraint, const Vector2D& textConstraint) {
    const auto CHILD_CONSTRAINT = dynamicPointerCast<CTextElement>(child) ? textConstraint : constraint;
    if (const auto PREFERRED = child->preferredSize(CHILD_CONSTRAINT))
        return *PREFERRED;
    if (const auto MINIMUM = child->minimumSize(CHILD_CONSTRAINT))
        return *MINIMUM;
    return {-1, -1};
}

SP<CRowLayoutElement> CRowLayoutElement::create(const SRowLayoutData& data) {
    auto p          = SP<CRowLayoutElement>(new CRowLayoutElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CRowLayoutElement::CRowLayoutElement(const SRowLayoutData& data) : IElement(), m_impl(makeUnique<SRowLayoutImpl>()) {
    m_impl->data = data;
}

void CRowLayoutElement::paint() {
    ; // no-op
}

void CRowLayoutElement::replaceData(const SRowLayoutData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CRowLayoutElement::reposition(const Hyprutils::Math::CBox& sbox, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(sbox);
    LinearLayout::reposition<true>(this, impl->position, impl->children, m_impl->data.gap, [this](SP<IElement> c) { return childSize(c); });
}

Hyprutils::Math::Vector2D CRowLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    const auto CONSTRAINT = impl->position.size();
    return childSizeForConstraint(child, CONSTRAINT, {-1.F, CONSTRAINT.y});
}

Hyprutils::Math::Vector2D CRowLayoutElement::size() {
    return impl->position.size();
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_impl->data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    const Vector2D TEXT_CONSTRAINT{-1.F, calc.y > 0 ? calc.y : -1.F};
    Vector2D       max;
    for (const auto& child : impl->children) {
        const auto CHILD_SIZE = childSizeForConstraint(child, parent, TEXT_CONSTRAINT);
        max.x += CHILD_SIZE.x + m_impl->data.gap;
        max.y = std::max(max.y, CHILD_SIZE.y);
    }

    if (!impl->children.empty())
        max.x -= m_impl->data.gap;

    max.x += impl->margin * 2;
    max.y += impl->margin * 2;

    max.x = std::ceil(max.x);
    max.y = std::ceil(max.y);

    if (calc.x == -1)
        calc.x = max.x;
    if (calc.y == -1)
        calc.y = max.y;

    return calc;
}

std::optional<Hyprutils::Math::Vector2D> CRowLayoutElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    const auto     CALC            = m_impl->data.size.calculate(parent);
    const Vector2D TEXT_CONSTRAINT = {-1.F, CALC.y > 0 ? CALC.y : -1.F};
    Vector2D       min;
    for (const auto& child : impl->children) {
        const auto CHILD_SIZE = childSizeForConstraint(child, parent, TEXT_CONSTRAINT);
        min.y                 = std::max(min.y, CHILD_SIZE.y);
    }

    return min;
}

bool CRowLayoutElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
