#include "Button.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

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

    m_impl->background = CRectangleBuilder::begin()
                             ->color([nobg = m_impl->data.noBg] {
                                 if (nobg)
                                     return CHyprColor{g_palette->m_colors.base.asRGB(), 0.F};
                                 return g_palette->m_colors.base;
                             })
                             ->rounding(5)
                             ->borderColor([] { return g_palette->m_colors.alternateBase; })
                             ->borderThickness(data.noBorder ? 0 : 1)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                             ->commence();

    m_impl->label = CTextBuilder::begin()
                        ->text(std::string{data.label})
                        ->fontSize(CFontSize{data.fontSize})
                        ->fontFamily(std::string{data.fontFamily})
                        ->color([] { return g_palette->m_colors.text; })
                        ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1.F, 1.F}})
                        ->callback([this] {
                            m_impl->labelChanged = true;
                            if (impl->window)
                                impl->window->scheduleReposition(impl->self);
                        })
                        ->commence();

    m_impl->label->setPositionMode(m_impl->data.alignText == HT_FONT_ALIGN_CENTER ? HT_POSITION_CENTER :
                                                                                    (m_impl->data.alignText == HT_FONT_ALIGN_RIGHT ? HT_POSITION_RIGHT : HT_POSITION_LEFT));

    addChild(m_impl->background);
    m_impl->background->addChild(m_impl->label);
    m_impl->label->setMargin(2);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_impl->background
            ->rebuild() //
            ->color([nb = m_impl->data.noBorder, nobg = m_impl->data.noBg] {
                if (nobg)
                    return g_palette->m_colors.base.brighten(0.05F);
                return g_palette->m_colors.base.brighten(nb ? 0.3F : 0.11F);
            })
            ->borderColor([] { return g_palette->m_colors.accent; })
            ->commence();
    });

    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_impl->background
            ->rebuild() //
            ->color([nobg = m_impl->data.noBg] {
                if (nobg)
                    return CHyprColor{g_palette->m_colors.base.asRGB(), 0.F};
                return g_palette->m_colors.base;
            })
            ->borderColor([] { return g_palette->m_colors.alternateBase; })
            ->commence();
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

    impl->grouped = true;
}

void CButtonElement::paint() {
    ;
}

void CButtonElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CButtonBuilder> CButtonElement::rebuild() {
    auto p       = SP<CButtonBuilder>(new CButtonBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SButtonData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CButtonElement::replaceData(const SButtonData& data) {
    m_impl->data = data;

    m_impl->label->rebuild()->text(std::string{data.label})->commence();

    m_impl->label->setPositionMode(m_impl->data.alignText == HT_FONT_ALIGN_CENTER ? HT_POSITION_CENTER :
                                                                                    (m_impl->data.alignText == HT_FONT_ALIGN_RIGHT ? HT_POSITION_RIGHT : HT_POSITION_LEFT));

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

    const auto CALC = m_impl->label->preferredSize(parent).value() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

std::optional<Vector2D> CButtonElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    const auto CALC = m_impl->label->preferredSize(parent).value() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

std::optional<Vector2D> CButtonElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    auto s = m_impl->data.size.calculate(parent);
    if (s.x != -1 && s.y != -1)
        return s;

    const auto CALC = m_impl->label->preferredSize(parent).value() + Vector2D{BUTTON_PAD * 2, BUTTON_PAD * 2};

    if (s.x == -1)
        s.x = CALC.x;
    if (s.y == -1)
        s.y = CALC.y;

    return s;
}

bool CButtonElement::acceptsMouseInput() {
    return true;
}

ePointerShape CButtonElement::pointerShape() {
    return HT_POINTER_POINTER;
}

bool CButtonElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
