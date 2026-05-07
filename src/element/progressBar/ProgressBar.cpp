#include "ProgressBar.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <algorithm>

#include "../../core/AnimationManager.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;
using CBAV = Hyprutils::Animation::CBaseAnimatedVariable;

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

    const float W = m_impl->data.indeterminate ? PROGRESSBAR_PULSE_WIDTH : std::clamp(m_impl->data.value, 0.F, 1.F);

    m_impl->foreground = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.accent; })
                             ->rounding(g_palette->m_vars.smallRounding)
                             ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {W, 1.F}})
                             ->commence();

    m_impl->background->addChild(m_impl->foreground);
    addChild(m_impl->background);

    impl->grouped = true;

    if (m_impl->data.indeterminate)
        m_impl->startIndeterminate();
}

void SProgressBarImpl::applyValue() {
    const float W = std::clamp(data.value, 0.F, 1.F);
    foreground->rebuild()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {W, 1.F}})->commence();
}

void SProgressBarImpl::startIndeterminate() {
    if (phase)
        return;

    foreground->setPositionMode(IElement::HT_POSITION_ABSOLUTE);

    g_animationManager->createAnimation(0.F, phase, g_animationManager->m_animationTree.getConfig("indeterminate"));

    phase->setUpdateCallback([this](WP<CBAV>) { onPulseTick(); });
    phase->setCallbackOnEnd(
        [this](WP<CBAV>) {
            phase->setValueAndWarp(0.F);
            *phase = 1.F;
        },
        false);

    *phase = 1.F;
}

void SProgressBarImpl::stopIndeterminate() {
    if (!phase)
        return;
    phase->resetAllCallbacks();
    phase.reset();

    foreground->setPositionMode(IElement::HT_POSITION_AUTO);
    foreground->rebuild()
        ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {std::clamp(data.value, 0.F, 1.F), 1.F}})
        ->commence();
}

void SProgressBarImpl::onPulseTick() {
    const float bgW = background->size().x;
    if (bgW <= 0.F)
        return;
    const float pulseW = bgW * PROGRESSBAR_PULSE_WIDTH;
    const float t      = phase->value();
    const float x      = t * (bgW + pulseW) - pulseW;
    foreground->setAbsolutePosition({x, 0.0});
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
    const bool INDET_CHANGED = data.indeterminate != m_impl->data.indeterminate;

    m_impl->data = data;

    if (INDET_CHANGED) {
        if (m_impl->data.indeterminate)
            m_impl->startIndeterminate();
        else
            m_impl->stopIndeterminate();
    } else if (VALUE_CHANGED && !m_impl->data.indeterminate)
        m_impl->applyValue();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

float CProgressBarElement::value() {
    return m_impl->data.value;
}

void CProgressBarElement::setValue(float v) {
    if (v == m_impl->data.value)
        return;
    m_impl->data.value = v;
    if (!m_impl->data.indeterminate)
        m_impl->applyValue();
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
