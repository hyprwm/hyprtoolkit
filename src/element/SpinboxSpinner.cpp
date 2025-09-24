#include "Spinbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../core/InternalBackend.hpp"
#include "../layout/Positioner.hpp"
#include "../renderer/Renderer.hpp"
#include "../window/ToolkitWindow.hpp"
#include "../core/AnimationManager.hpp"
#include "Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CSpinboxSpinner> CSpinboxSpinner::create(SP<CSpinboxElement> data) {
    auto p        = SP<CSpinboxSpinner>(new CSpinboxSpinner(data));
    p->impl->self = p;
    return p;
}

CSpinboxSpinner::CSpinboxSpinner(SP<CSpinboxElement> data) : IElement(), m_parent(data) {
    m_layout = CRowLayoutElement::create(SRowLayoutData{
        .size = {CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}},
        .gap  = 3,
    });

    m_layout->setPositionMode(HT_POSITION_CENTER);

    m_label = CTextElement::create(STextData{
        .text     = m_parent->m_impl->data.items.at(m_parent->m_impl->data.currentItem),
        .color    = [] { return g_palette->m_colors.text; },
        .callback = [this] { g_positioner->repositionNeeded(m_parent); },
    });

    m_background = CRectangleElement::create(SRectangleData{
        .color           = [] { return g_palette->m_colors.base; },
        .rounding        = 6,
        .borderColor     = [] { return g_palette->m_colors.alternateBase; },
        .borderThickness = 1,
        .size            = CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}},
    });

    m_background->setPositionMode(HT_POSITION_CENTER);

    m_left = CTextElement::create(STextData{
        .text     = "<",
        .fontSize = CFontSize{CFontSize::HT_FONT_H3},
        .color    = [] { return g_palette->m_colors.text; },
        .callback = [this] { g_positioner->repositionNeeded(m_parent); },
    });

    m_right = CTextElement::create(STextData{
        .text     = ">",
        .fontSize = CFontSize{CFontSize::HT_FONT_H3},
        .color    = [] { return g_palette->m_colors.text; },
        .callback = [this] { g_positioner->repositionNeeded(m_parent); },
    });

    m_layout->addChild(m_left);
    m_layout->addChild(m_label);
    m_layout->addChild(m_right);

    addChild(m_background);
    addChild(m_layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_primedForUp  = false;
        m_lastPosLocal = pos;
    });
    impl->m_externalEvents.mouseMove.listenStatic([this](const Vector2D& pos) { m_lastPosLocal = pos; });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() { m_primedForUp = false; });

    impl->m_externalEvents.mouseButton.listenStatic([this](const Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        if (down) {
            m_primedForUp = true;
            return;
        }

        if (!m_primedForUp)
            return;

        if (button == Input::MOUSE_BUTTON_LEFT) {
            // check if we are on the right or left
            if (m_left->impl->position.containsPoint(m_lastPosLocal + impl->position.pos())) {
                // switch left
                if (m_parent->m_impl->data.currentItem)
                    m_parent->setCurrent(m_parent->m_impl->data.currentItem - 1);
                else
                    m_parent->setCurrent(m_parent->m_impl->data.items.size() - 1);
            } else if (m_right->impl->position.containsPoint(m_lastPosLocal + impl->position.pos())) {
                // switch right
                if (m_parent->m_impl->data.currentItem)
                    m_parent->setCurrent(m_parent->m_impl->data.currentItem - 1);
                else
                    m_parent->setCurrent(m_parent->m_impl->data.items.size() - 1);
            }
        }
    });
}

void CSpinboxSpinner::paint() {
    ;
}

void CSpinboxSpinner::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CSpinboxSpinner::size() {
    return impl->position.size();
}

std::optional<Vector2D> CSpinboxSpinner::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CSpinboxSpinner::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CSpinboxSpinner::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

bool CSpinboxSpinner::acceptsMouseInput() {
    return true;
}

ePointerShape CSpinboxSpinner::pointerShape() {
    return HT_POINTER_POINTER;
}
