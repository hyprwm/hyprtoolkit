#include "Slider.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <cmath>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CSliderElement> CSliderElement::create(const SSliderData& data) {
    auto p          = SP<CSliderElement>(new CSliderElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CSliderElement::CSliderElement(const SSliderData& data) : IElement(), m_impl(makeUnique<SSliderImpl>()) {
    m_impl->data = data;

    m_impl->layout = CRowLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();

    m_impl->layout->setPositionMode(HT_POSITION_CENTER);

    m_impl->textContainer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {m_impl->maxLabelSize(), 1.F}})->commence();

    m_impl->valueText = CTextBuilder::begin() //
                            ->text(m_impl->valueAsText())
                            ->color([] { return g_palette->m_colors.text; })
                            ->callback([this] { impl->window->scheduleReposition(impl->self); })
                            ->commence();

    m_impl->valueText->setPositionMode(HT_POSITION_CENTER);
    m_impl->textContainer->addChild(m_impl->valueText);

    m_impl->background = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.base; })
                             ->rounding(4)
                             ->borderColor([] { return g_palette->m_colors.alternateBase; })
                             ->borderThickness(1)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                             ->commence();

    m_impl->background->setPositionMode(HT_POSITION_CENTER);

    m_impl->foreground = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.accent; })
                             ->rounding(4)
                             ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {(m_impl->data.current / m_impl->data.max), 1.F}})
                             ->commence();

    m_impl->background->addChild(m_impl->foreground);

    m_impl->layout->addChild(m_impl->background);
    m_impl->layout->addChild(m_impl->textContainer);

    addChild(m_impl->layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_impl->dragging     = false;
        m_impl->lastPosLocal = pos;

        m_impl->background->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })->commence();
    });
    impl->m_externalEvents.mouseMove.listenStatic([this](const Vector2D& pos) {
        m_impl->lastPosLocal = pos;
        if (m_impl->dragging)
            m_impl->updateValue();
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_impl->dragging = false;
        m_impl->background->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase; })->commence();
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        m_impl->dragging = down;

        if (!m_impl->dragging)
            return;

        m_impl->updateValue();
    });
}

void SSliderImpl::valueChanged(float perc) {
    data.current = data.max * perc;
    foreground->rebuild()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {(data.current / data.max), 1.F}})->commence();

    textContainer->rebuild()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {maxLabelSize(), 1.F}})->commence();

    valueText->rebuild()->text(valueAsText())->commence();
}

void SSliderImpl::updateValue() {
    CBox box = background->impl->position;
    // expand grab area for the user.
    box.h += 10;
    box.y -= 5;

    const float CURRENT_VALUE =
        std::clamp(sc<float>(((lastPosLocal + background->impl->position.pos()).x - background->impl->position.pos().x) / background->impl->position.size().x), 0.F, 1.F);

    valueChanged(CURRENT_VALUE);

    if (data.onChanged)
        data.onChanged(self.lock(), CURRENT_VALUE);
}

void CSliderElement::paint() {
    ;
}

void CSliderElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CSliderBuilder> CSliderElement::rebuild() {
    auto p       = SP<CSliderBuilder>(new CSliderBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SSliderData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CSliderElement::replaceData(const SSliderData& data) {
    const bool VALUE_CHANGED = data.current != m_impl->data.current;

    m_impl->data = data;

    if (VALUE_CHANGED) {
        m_impl->valueChanged(data.current);

        if (data.onChanged)
            data.onChanged(m_impl->self.lock(), m_impl->data.current);
    }

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

float SSliderImpl::maxLabelSize() {
    // TODO: maybe actually calculate it or something?
    size_t maxChars = std::floor(std::log10(data.max));

    if (!data.snapInt)
        maxChars += 2;

    return maxChars * 10.F;
}

std::string SSliderImpl::valueAsText() {
    if (data.snapInt)
        return std::format("{}", sc<int>(std::round(data.current)));

    return std::format("{:.1f}", data.current);
}

Hyprutils::Math::Vector2D CSliderElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CSliderElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);

    if (s.x != -1 && s.y != -1)
        return s;

    const auto CALC = m_impl->layout->preferredSize(parent).value() + Vector2D{1, 1};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

std::optional<Vector2D> CSliderElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    const auto CALC = m_impl->layout->preferredSize(parent).value() + Vector2D{1, 1};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

std::optional<Vector2D> CSliderElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    const auto CALC = m_impl->layout->preferredSize(parent).value() + Vector2D{1, 1};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

bool CSliderElement::acceptsMouseInput() {
    return true;
}

ePointerShape CSliderElement::pointerShape() {
    return HT_POINTER_POINTER;
}
