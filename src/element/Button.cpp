#include "Button.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CButtonElement> CButtonElement::create(const SButtonData& data) {
    auto p          = SP<CButtonElement>(new CButtonElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CButtonElement::CButtonElement(const SButtonData& data) : IElement(), m_impl(makeUnique<SButtonImpl>()) {
    m_impl->data = data;

    m_impl->background = CRectangleElement::create(SRectangleData{
        .color           = g_palette->m_colors.base,
        .rounding        = 5,
        .borderColor     = g_palette->m_colors.alternateBase,
        .borderThickness = 1,
        .size            = CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}},
    });

    m_impl->label = CTextElement::create(STextData{
        .text  = data.label,
        .color = g_palette->m_colors.text,
        .size  = CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}},
        .callback =
            [this] {
                m_impl->labelChanged = true;
                g_positioner->repositionNeeded(impl->self.lock());
            },
    });

    addChild(m_impl->background);
    addChild(m_impl->label);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        auto bg        = m_impl->background->dataCopy();
        bg.color       = g_palette->m_colors.base.brighten(0.11F);
        bg.borderColor = g_palette->m_colors.accent;
        m_impl->background->replaceData(bg);
    });

    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        auto bg        = m_impl->background->dataCopy();
        bg.color       = g_palette->m_colors.base;
        bg.borderColor = g_palette->m_colors.alternateBase;
        m_impl->background->replaceData(bg);
    });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (!down)
            return;

        if (button == Input::MOUSE_BUTTON_RIGHT) {
            if (m_impl->data.onRightClick)
                m_impl->data.onRightClick(m_impl->self.lock());
        } else if (button == Input::MOUSE_BUTTON_LEFT) {
            if (m_impl->data.onMainClick)
                m_impl->data.onMainClick(m_impl->self.lock());
        }
    });
}

void CButtonElement::paint() {
    ;
}

void CButtonElement::reposition(const Hyprutils::Math::CBox& box) {
    IElement::reposition(box);

    g_positioner->position(m_impl->background, impl->position);

    // center label box in box
    auto labelBox = CBox{
        box.pos(),
        m_impl->label->impl->position.size(),
    };

    const bool LABEL_CHANGED = m_impl->labelChanged;

    if (LABEL_CHANGED) {
        m_impl->labelChanged = false;
        g_positioner->position(m_impl->label, impl->position);
    }

    labelBox.translate((box.size() - m_impl->label->impl->position.size()) / 2.F);

    g_positioner->position(m_impl->label, labelBox);

    if (LABEL_CHANGED)
        g_positioner->position(impl->parent.lock(), impl->parent->impl->position);
}

SButtonData CButtonElement::dataCopy() {
    return m_impl->data;
}

void CButtonElement::replaceData(const SButtonData& data) {
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

Hyprutils::Math::Vector2D CButtonElement::size() {
    return impl->position.size();
}

constexpr double        BUTTON_PAD = 5;

std::optional<Vector2D> CButtonElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);

    if (s.x != -1 && s.y != -1)
        return s;

    return m_impl->label->impl->position.size() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};
}

std::optional<Vector2D> CButtonElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    return m_impl->label->impl->position.size() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};
}

std::optional<Vector2D> CButtonElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    return m_impl->label->impl->position.size() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};
}

bool CButtonElement::acceptsMouseInput() {
    return true;
}

ePointerShape CButtonElement::pointerShape() {
    return HT_POINTER_POINTER;
}
