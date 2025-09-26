#include "Spinbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CSpinboxSpinner> CSpinboxSpinner::create(SP<CSpinboxElement> data) {
    auto p        = SP<CSpinboxSpinner>(new CSpinboxSpinner(data));
    p->impl->self = p;
    p->m_self     = p;
    p->init();
    return p;
}

constexpr float ANGLE_SIZE = 12.F;
constexpr float INNER_MARG = 2.F;

CSpinboxSpinner::CSpinboxSpinner(SP<CSpinboxElement> data) : IElement(), m_parent(data) {
    ;
}

void CSpinboxSpinner::init() {
    m_layout = CRowLayoutBuilder::begin()->gap(6)->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    m_layout->setPositionMode(HT_POSITION_CENTER);
    m_layout->setMargin(INNER_MARG);

    m_label = CTextBuilder::begin()
                  ->text(std::string{m_parent->m_impl->data.items.at(m_parent->m_impl->data.currentItem)})
                  ->color([] { return g_palette->m_colors.text; })
                  ->callback([this] { impl->window->scheduleReposition(impl->self); })
                  ->commence();

    m_background = CRectangleBuilder::begin()
                       ->color([] { return g_palette->m_colors.base; })
                       ->rounding(4)
                       ->borderColor([] { return g_palette->m_colors.alternateBase; })
                       ->borderThickness(1)
                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                       ->commence();

    m_background->setPositionMode(HT_POSITION_CENTER);

    m_left = CSpinboxAngleElement::create(SSpinboxAngleData{
        .size        = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {ANGLE_SIZE, ANGLE_SIZE}},
        .color       = [] { return g_palette->m_colors.text; },
        .colorActive = [] { return g_palette->m_colors.text.mix(g_palette->m_colors.accent, 0.75F); },
        .spinner     = m_self,
    });

    m_right = CSpinboxAngleElement::create(SSpinboxAngleData{
        .size        = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {ANGLE_SIZE, ANGLE_SIZE}},
        .color       = [] { return g_palette->m_colors.text; },
        .colorActive = [] { return g_palette->m_colors.text.mix(g_palette->m_colors.accent, 0.75F); },
        .right       = true,
        .spinner     = m_self,
    });

    m_leftPad  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {0.F, 1.F}})->commence();
    m_rightPad = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {0.F, 1.F}})->commence();

    m_layout->addChild(m_leftPad);
    m_layout->addChild(m_left);
    m_layout->addChild(m_label);
    m_layout->addChild(m_right);
    m_layout->addChild(m_rightPad);

    addChild(m_background);
    addChild(m_layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })
            ->commence();
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase; })
            ->commence();
    });
}

void CSpinboxSpinner::moveSelection(bool forward) {
    if (!forward) {
        // switch left
        if (m_parent->m_impl->data.currentItem)
            m_parent->setCurrent(m_parent->m_impl->data.currentItem - 1);
        else
            m_parent->setCurrent(m_parent->m_impl->data.items.size() - 1);
    } else {
        // switch right
        if (m_parent->m_impl->data.currentItem)
            m_parent->setCurrent(m_parent->m_impl->data.currentItem - 1);
        else
            m_parent->setCurrent(m_parent->m_impl->data.items.size() - 1);
    }
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
    return HT_POINTER_ARROW;
}

bool CSpinboxSpinner::alwaysGetMouseInput() {
    return true;
}
