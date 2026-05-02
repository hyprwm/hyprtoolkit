#include "ProgressBar.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <algorithm>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

SP<CProgressBarElement> CProgressBarElement::create(const SProgressBarData& data) {
    auto p          = SP<CProgressBarElement>(new CProgressBarElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CProgressBarElement::CProgressBarElement(const SProgressBarData& data) : IElement(), m_impl(makeUnique<SProgressBarImpl>()) {
    m_impl->data = data;

    m_impl->background = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.base; })
                             ->rounding(g_palette->m_vars.smallRounding)
                             ->borderColor([] { return g_palette->m_colors.alternateBase; })
                             ->borderThickness(1)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                             ->commence();

    m_impl->background->setPositionMode(HT_POSITION_ABSOLUTE);

    const float W = std::clamp(m_impl->data.value, 0.F, 1.F);

    m_impl->foreground = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.accent; })
                             ->rounding(g_palette->m_vars.smallRounding)
                             ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {W, 1.F}})
                             ->commence();

    m_impl->background->addChild(m_impl->foreground);
    addChild(m_impl->background);

    impl->grouped = true;
}

void SProgressBarImpl::applyValue() {
    const float W = std::clamp(data.value, 0.F, 1.F);
    foreground->rebuild()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {W, 1.F}})->commence();
}

void CProgressBarElement::paint() {
    ;
}

void CProgressBarElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);
    g_positioner->positionChildren(impl->self.lock());
}

SP<CProgressBarBuilder> CProgressBarElement::rebuild() {
    auto p       = SP<CProgressBarBuilder>(new CProgressBarBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SProgressBarData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CProgressBarElement::replaceData(const SProgressBarData& data) {
    const bool VALUE_CHANGED = data.value != m_impl->data.value;

    m_impl->data = data;

    if (VALUE_CHANGED)
        m_impl->applyValue();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

Hyprutils::Math::Vector2D CProgressBarElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CProgressBarElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CProgressBarElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CProgressBarElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

bool CProgressBarElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
