#include "Checkbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CCheckboxElement> CCheckboxElement::create(const SCheckboxData& data) {
    auto p          = SP<CCheckboxElement>(new CCheckboxElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

std::function<CHyprColor()> SCheckboxImpl::getFgColor() {
    if (data.toggled)
        return [] { return g_palette->m_colors.accent; };
    else {
        return [] {
            auto c = g_palette->m_colors.accent;
            c.a    = 0.F;
            return c;
        };
    }
}

CCheckboxElement::CCheckboxElement(const SCheckboxData& data) : IElement(), m_impl(makeUnique<SCheckboxImpl>()) {
    m_impl->data = data;

    m_impl->layout = CRowLayoutElement::create(SRowLayoutData{
        .size = {data.fill ? CDynamicSize::HT_SIZE_PERCENT : CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
        .gap  = 3,
    });

    m_impl->background = CRectangleElement::create(SRectangleData{
        .color           = [] { return g_palette->m_colors.base; },
        .rounding        = 4,
        .borderColor     = [] { return g_palette->m_colors.alternateBase; },
        .borderThickness = 1,
        .size            = CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {14.F, 14.F}},
    });

    m_impl->background->setPositionMode(HT_POSITION_CENTER);

    m_impl->foreground = CRectangleElement::create(SRectangleData{
        .color    = m_impl->getFgColor(),
        .rounding = 0,
        .size     = CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}},
    });

    m_impl->foreground->setMargin(4);
    m_impl->foreground->setPositionMode(HT_POSITION_CENTER);

    m_impl->label = CTextElement::create(STextData{
        .text  = data.label,
        .color = [] { return g_palette->m_colors.text; },
        .size  = CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}},
        .callback =
            [this] {
                m_impl->labelChanged = true;
                g_positioner->repositionNeeded(impl->self.lock());
            },
    });

    m_impl->spacer = CNullElement::create();
    m_impl->spacer->setGrow(true, true);

    m_impl->layout->addChild(m_impl->label);
    m_impl->layout->addChild(m_impl->spacer);
    m_impl->layout->addChild(m_impl->background);
    m_impl->background->addChild(m_impl->foreground);

    addChild(m_impl->layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        auto bg        = m_impl->background->dataCopy();
        bg.color       = [] { return g_palette->m_colors.base.brighten(0.11F); };
        bg.borderColor = [] { return g_palette->m_colors.accent; };
        m_impl->background->replaceData(bg);
        m_impl->primedForUp = false;
    });

    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        auto bg        = m_impl->background->dataCopy();
        bg.color       = [] { return g_palette->m_colors.base; };
        bg.borderColor = [] { return g_palette->m_colors.alternateBase; };
        m_impl->background->replaceData(bg);
        m_impl->primedForUp = false;
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (down) {
            m_impl->primedForUp = true;
            return;
        }

        if (!m_impl->primedForUp)
            return;

        if (button == Input::MOUSE_BUTTON_LEFT) {
            m_impl->data.toggled = !m_impl->data.toggled;

            if (m_impl->data.onToggled)
                m_impl->data.onToggled(m_impl->self.lock(), m_impl->data.toggled);

            auto fg  = m_impl->foreground->dataCopy();
            fg.color = m_impl->getFgColor();

            m_impl->foreground->replaceData(fg);
        }
    });
}

void CCheckboxElement::paint() {
    ;
}

void CCheckboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SCheckboxData CCheckboxElement::dataCopy() {
    return m_impl->data;
}

void CCheckboxElement::replaceData(const SCheckboxData& data) {
    m_impl->data = data;

    auto labelData = m_impl->label->dataCopy();

    if (labelData.text != data.label) {
        labelData.text = data.label;
        m_impl->label->replaceData(labelData);
        m_impl->labelChanged = true;
    }

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

Hyprutils::Math::Vector2D CCheckboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CCheckboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CCheckboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CCheckboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
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

bool CCheckboxElement::acceptsMouseInput() {
    return true;
}

ePointerShape CCheckboxElement::pointerShape() {
    return HT_POINTER_POINTER;
}
