#include "Slider.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

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
    p->init();
    return p;
}

CSliderElement::CSliderElement(const SSliderData& data) : IElement(), m_impl(makeUnique<SSliderImpl>()) {
    m_impl->data = data;
}

void CSliderElement::init() {
    m_impl->layout =
        CRowLayoutBuilder::begin()->gap(3)->size({m_impl->data.fill ? CDynamicSize::HT_SIZE_PERCENT : CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    m_impl->label = CTextBuilder::begin() //
                        ->text(std::string{m_impl->data.label})
                        ->color([] { return g_palette->m_colors.text; })
                        ->callback([this] { impl->window->scheduleReposition(impl->self); })
                        ->commence();

    m_impl->slider = CSliderSlider::create(m_impl->self.lock());

    m_impl->spacer = CNullBuilder::begin()->commence();

    m_impl->spacer->setGrow(true);

    m_impl->layout->addChild(m_impl->label);
    m_impl->layout->addChild(m_impl->spacer);
    m_impl->layout->addChild(m_impl->slider);

    addChild(m_impl->layout);
}

void CSliderElement::valueChanged(float perc) {
    m_impl->data.current = m_impl->data.max * perc;
    m_impl->slider->valueChanged(perc);
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
    m_impl->data = data;

    m_impl->label->rebuild()->text(std::string{data.label})->commence();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
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
    return false;
}

ePointerShape CSliderElement::pointerShape() {
    return HT_POINTER_POINTER;
}
