#include "Spinbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

#include "../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CSpinboxElement> CSpinboxElement::create(const SSpinboxData& data) {
    auto p          = SP<CSpinboxElement>(new CSpinboxElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    p->init();
    return p;
}

CSpinboxElement::CSpinboxElement(const SSpinboxData& data) : IElement(), m_impl(makeUnique<SSpinboxImpl>()) {
    m_impl->data = data;
}

void CSpinboxElement::init() {
    RASSERT(!m_impl->data.items.empty(), "Spinbox can't be empty");

    m_impl->layout = CRowLayoutElement::create(SRowLayoutData{
        .size = {m_impl->data.fill ? CDynamicSize::HT_SIZE_PERCENT : CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
        .gap  = 3,
    });

    m_impl->label = CTextElement::create(STextData{
        .text  = m_impl->data.label,
        .color = [] { return g_palette->m_colors.text; },
        .size  = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
    });

    m_impl->spinner = CSpinboxSpinner::create(m_impl->self.lock());

    m_impl->spacer = CNullElement::create();
    m_impl->spacer->setGrow(true, true);

    m_impl->layout->addChild(m_impl->label);
    m_impl->layout->addChild(m_impl->spacer);
    m_impl->layout->addChild(m_impl->spinner);

    addChild(m_impl->layout);
}

void CSpinboxElement::paint() {
    ;
}

void CSpinboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SSpinboxData CSpinboxElement::dataCopy() {
    return m_impl->data;
}

void CSpinboxElement::replaceData(const SSpinboxData& data) {
    m_impl->data = data;

    auto labelData = m_impl->label->dataCopy();

    if (labelData.text != data.label) {
        labelData.text = data.label;
        m_impl->label->replaceData(labelData);
    }

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

size_t CSpinboxElement::current() {
    return m_impl->data.currentItem;
}

void CSpinboxElement::setCurrent(size_t current) {
    m_impl->data.currentItem = std::min(current, m_impl->data.items.size() - 1);

    auto d = m_impl->spinner->m_label->dataCopy();
    d.text = m_impl->data.items.at(m_impl->data.currentItem);
    m_impl->spinner->m_label->replaceData(d);
}

Hyprutils::Math::Vector2D CSpinboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CSpinboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CSpinboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CSpinboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
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

bool CSpinboxElement::acceptsMouseInput() {
    return false;
}

ePointerShape CSpinboxElement::pointerShape() {
    return HT_POINTER_POINTER;
}
