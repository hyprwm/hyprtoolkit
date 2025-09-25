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

SP<CSliderSlider> CSliderSlider::create(SP<CSliderElement> data) {
    auto p        = SP<CSliderSlider>(new CSliderSlider(data));
    p->impl->self = p;
    return p;
}

constexpr float BAR_SIZE   = 150.F;
constexpr float BAR_HEIGHT = 8.F;

CSliderSlider::CSliderSlider(SP<CSliderElement> data) : IElement(), m_parent(data) {

    m_layout = CRowLayoutBuilder::begin()->gap(3)->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    m_layout->setPositionMode(HT_POSITION_CENTER);

    m_textContainer = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {maxLabelSize(), 1.F}})->commence();

    m_valueText = CTextBuilder::begin() //
                      ->text(valueAsText())
                      ->color([] { return g_palette->m_colors.text; })
                      ->callback([this] { g_positioner->repositionNeeded(impl->self.lock()); })
                      ->commence();

    m_valueText->setPositionMode(HT_POSITION_CENTER);
    m_textContainer->addChild(m_valueText);

    m_background = CRectangleBuilder::begin()
                       ->color([] { return g_palette->m_colors.base; })
                       ->rounding(BAR_HEIGHT / 2.F)
                       ->borderColor([] { return g_palette->m_colors.alternateBase; })
                       ->borderThickness(1)
                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {BAR_SIZE, BAR_HEIGHT}})
                       ->commence();

    m_background->setPositionMode(HT_POSITION_CENTER);

    m_foreground = CRectangleBuilder::begin()
                       ->color([] { return g_palette->m_colors.accent; })
                       ->rounding(BAR_HEIGHT / 2.F)
                       ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {BAR_SIZE * (m_parent->m_impl->data.current / m_parent->m_impl->data.max), 1.F}})
                       ->commence();

    m_background->addChild(m_foreground);

    m_layout->addChild(m_background);
    m_layout->addChild(m_textContainer);

    addChild(m_layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_dragging     = false;
        m_lastPosLocal = pos;

        m_background->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })->commence();
    });
    impl->m_externalEvents.mouseMove.listenStatic([this](const Vector2D& pos) {
        m_lastPosLocal = pos;
        if (m_dragging)
            updateValue();
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_dragging = false;
        m_background->rebuild()->borderColor([] { return g_palette->m_colors.alternateBase; })->commence();
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        m_dragging = down;

        if (!m_dragging)
            return;

        updateValue();
    });
}

float CSliderSlider::maxLabelSize() {
    // TODO: maybe actually calculate it or something?
    size_t maxChars = std::floor(std::log10(m_parent->m_impl->data.max));

    if (!m_parent->m_impl->data.snapInt)
        maxChars += 2;

    return maxChars * 10.F;
}

std::string CSliderSlider::valueAsText() {
    if (m_parent->m_impl->data.snapInt)
        return std::format("{}", sc<int>(std::round(m_parent->m_impl->data.current)));

    return std::format("{:.1f}", m_parent->m_impl->data.current);
}

void CSliderSlider::valueChanged(float perc) {
    m_foreground->rebuild()
        ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {BAR_SIZE * (m_parent->m_impl->data.current / m_parent->m_impl->data.max), 1.F}})
        ->commence();

    m_textContainer->rebuild()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {maxLabelSize(), 1.F}})->commence();

    m_valueText->rebuild()->text(valueAsText())->commence();
}

void CSliderSlider::updateValue() {
    CBox box = m_background->impl->position;
    // expand grab area for the user.
    box.h += 10;
    box.y -= 5;

    const float CURRENT_VALUE =
        std::clamp(sc<float>(((m_lastPosLocal + m_background->impl->position.pos()).x - m_background->impl->position.pos().x) / m_background->impl->position.size().x), 0.F, 1.F);

    m_parent->valueChanged(CURRENT_VALUE);
}

void CSliderSlider::paint() {
    ;
}

void CSliderSlider::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CSliderSlider::size() {
    return impl->position.size();
}

std::optional<Vector2D> CSliderSlider::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CSliderSlider::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CSliderSlider::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

bool CSliderSlider::acceptsMouseInput() {
    return true;
}

ePointerShape CSliderSlider::pointerShape() {
    return HT_POINTER_POINTER;
}
