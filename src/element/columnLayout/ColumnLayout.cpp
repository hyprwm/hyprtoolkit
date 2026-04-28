#include "ColumnLayout.hpp"
#include <cmath>

#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"

#include "../Element.hpp"
#include "../LinearLayout.hpp"

using namespace Hyprtoolkit;

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
    if (child->preferredSize(impl->position.size()))
        return *child->preferredSize(impl->position.size());
    else if (child->minimumSize(impl->position.size()))
        return *child->minimumSize(impl->position.size());
    return {-1, -1};
}

std::optional<Hyprutils::Math::Vector2D> CColumnLayoutElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto calc = m_impl->data.size.calculate(parent);

    if (calc.x != -1 && calc.y != -1)
        return calc;

    Vector2D max;
    for (const auto& child : impl->children) {
        max.x = std::max(childSize(child).x, max.x);
        max.y += childSize(child).y + m_impl->data.gap;
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
    Vector2D min;
    for (const auto& child : impl->children) {
        min.x = std::max(min.x, childSize(child).x);
    }

    return min;
}

bool CColumnLayoutElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
