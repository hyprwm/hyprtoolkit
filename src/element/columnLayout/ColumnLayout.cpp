#include "ColumnLayout.hpp"
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

SP<CColumnLayoutElement> CColumnLayoutElement::create(const SColumnLayoutData& data) {
    auto p          = SP<CColumnLayoutElement>(new CColumnLayoutElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CColumnLayoutElement::CColumnLayoutElement(const SColumnLayoutData& data) : IElement(), m_impl(makeUnique<SColumnLayoutImpl>()) {
    m_impl->data = data;
}

void CColumnLayoutElement::paint() {
    ; // no-op
}

SP<CColumnLayoutBuilder> CColumnLayoutElement::rebuild() {
    auto p       = SP<CColumnLayoutBuilder>(new CColumnLayoutBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SColumnLayoutData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CColumnLayoutElement::replaceData(const SColumnLayoutData& data) {
    m_impl->data = data;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CColumnLayoutElement::reposition(const Hyprutils::Math::CBox& sbox, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(sbox);
    LinearLayout::reposition<false>(this, impl->position, impl->children, m_impl->data.gap, [this](SP<IElement> c) { return childSize(c); });
}

Hyprutils::Math::Vector2D CColumnLayoutElement::size() {
    return impl->position.size();
}

Hyprutils::Math::Vector2D CColumnLayoutElement::childSize(Hyprutils::Memory::CSharedPointer<IElement> child) {
    const auto CONSTRAINT = impl->position.size();
    return childSizeForConstraint(child, CONSTRAINT, {CONSTRAINT.x, -1.F});
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_impl->data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    const Vector2D TEXT_CONSTRAINT{calc.x > 0 ? std::max(1.0, calc.x - impl->margin * 2.F) : -1.F, -1.F};
    Vector2D       max;
    for (const auto& child : impl->children) {
        const auto CHILD_SIZE = childSizeForConstraint(child, parent, TEXT_CONSTRAINT);
        max.x                 = std::max(CHILD_SIZE.x, max.x);
        max.y += CHILD_SIZE.y + m_impl->data.gap;
    }

    if (!impl->children.empty())
        max.y -= m_impl->data.gap;

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

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    const auto     CALC            = m_impl->data.size.calculate(parent);
    const Vector2D TEXT_CONSTRAINT = {CALC.x > 0 ? std::max(1.0, CALC.x - impl->margin * 2.F) : -1.F, -1.F};
    Vector2D       min;
    for (const auto& child : impl->children) {
        const auto CHILD_SIZE = childSizeForConstraint(child, parent, TEXT_CONSTRAINT);
        min.x                 = std::max(min.x, CHILD_SIZE.x);
    }

    return min;
}

bool CColumnLayoutElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
