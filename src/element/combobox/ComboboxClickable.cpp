#include "Combobox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/element/Button.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;
using namespace Hyprutils::Math;

SP<CComboboxClickable> CComboboxClickable::create(SP<CComboboxElement> data) {
    auto p        = SP<CComboboxClickable>(new CComboboxClickable(data));
    p->impl->self = p;
    p->m_self     = p;
    p->init();
    return p;
}

constexpr float INNER_MARG             = 2.F;
constexpr float HANDLE_SIZE            = 12.F;
constexpr float DROPDOWN_BUTTON_HEIGHT = 30.F;
constexpr float DROPDOWN_BUTTON_PAD    = 2.F;

CComboboxClickable::CComboboxClickable(SP<CComboboxElement> data) : IElement(), m_parent(data) {
    ;
}

void CComboboxClickable::init() {
    m_layout = CRowLayoutBuilder::begin()->gap(6)->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 24}})->commence();

    m_layout->setPositionMode(HT_POSITION_CENTER);
    m_layout->setMargin(INNER_MARG);

    m_label = CTextBuilder::begin()
                  ->text(std::string{m_parent->m_impl->data.items.at(m_parent->m_impl->data.currentItem)})
                  ->color([] { return g_palette->m_colors.text; })
                  ->callback([this] { g_positioner->repositionNeeded(m_parent); })
                  ->commence();

    m_background = CRectangleBuilder::begin()
                       ->color([] { return g_palette->m_colors.base; })
                       ->rounding(4)
                       ->borderColor([] { return g_palette->m_colors.alternateBase; })
                       ->borderThickness(1)
                       ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                       ->commence();

    m_background->setPositionMode(HT_POSITION_CENTER);

    m_handle = CDropdownHandleElement::create(SDropdownHandleData{
        .size  = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {HANDLE_SIZE, HANDLE_SIZE}},
        .color = [] { return g_palette->m_colors.text; },
    });

    m_leftPad  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {INNER_MARG, 1.F}})->commence();
    m_rightPad = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {INNER_MARG, 1.F}})->commence();

    m_layout->addChild(m_leftPad);
    m_layout->addChild(m_label);
    m_layout->addChild(m_handle);
    m_layout->addChild(m_rightPad);

    addChild(m_background);
    addChild(m_layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_primedForUp = false;
        m_background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })
            ->commence();
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_primedForUp = false;
        m_background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase; })
            ->commence();
    });

    m_listeners.clicked = impl->m_externalEvents.mouseButton.listen([this](Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        if (down) {
            m_primedForUp = true;
            return;
        }

        if (!m_primedForUp)
            return;

        openDropdown();
    });
}

void CComboboxClickable::openDropdown() {
    if (m_dropdown.popup)
        return;

    const Vector2D POPUP_SIZE = Vector2D{
        100.F + impl->position.size().x + (2 * DROPDOWN_BUTTON_PAD),
        std::min(                                                                                                                                                           //
            (DROPDOWN_BUTTON_HEIGHT * m_parent->m_impl->data.items.size()) + (DROPDOWN_BUTTON_PAD * (m_parent->m_impl->data.items.size() - 1)) + (2 * DROPDOWN_BUTTON_PAD), //
            600.F                                                                                                                                                           //
            ),
    };

    m_dropdown.popup = impl->window->openPopup(SPopupCreationData{
        .pos  = impl->position.pos() - Vector2D{50.F, 0.F} + Vector2D{0.F, impl->position.size().y},
        .size = POPUP_SIZE,
    });

    m_dropdown.popup->m_events.popupClosed.listenStatic([this, self = m_self] {
        if (!self)
            return;

        m_dropdown = {};
    });

    m_dropdown.scroll = CScrollAreaBuilder::begin()->scrollY(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();

    m_dropdown.layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(DROPDOWN_BUTTON_PAD)->commence();
    m_dropdown.layout->setMargin(DROPDOWN_BUTTON_PAD);

    m_dropdown.background = CRectangleBuilder::begin()
                                ->color([] { return g_palette->m_colors.background; })
                                ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                                ->rounding(10)
                                ->borderColor([] { return g_palette->m_colors.alternateBase; })
                                ->borderThickness(1)
                                ->commence();

    m_dropdown.background->setMargin(1);

    m_dropdown.popup->m_rootElement->addChild(m_dropdown.background);
    m_dropdown.background->addChild(m_dropdown.scroll);
    m_dropdown.scroll->addChild(m_dropdown.layout);

    m_dropdown.buttons.clear();

    for (size_t i = 0; i < m_parent->m_impl->data.items.size(); ++i) {
        m_dropdown.buttons.emplace_back(CButtonBuilder::begin()
                                            ->label(std::string{m_parent->m_impl->data.items.at(i)})
                                            ->noBorder(true)
                                            ->noBg(true)
                                            ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, DROPDOWN_BUTTON_HEIGHT}})
                                            ->onMainClick([i, this, self = m_self](SP<CButtonElement> e) {
                                                if (!self)
                                                    return;

                                                setSelection(i);

                                                g_backend->addIdle([this, self = m_self] {
                                                    if (!self)
                                                        return;

                                                    closeDropdown();
                                                });
                                            })
                                            ->commence() //
        );

        m_dropdown.layout->addChild(m_dropdown.buttons.back());
    }

    m_dropdown.popup->open();
}

void CComboboxClickable::closeDropdown() {
    if (!m_dropdown.popup)
        return;

    m_dropdown.popup->close();
}

void CComboboxClickable::setSelection(size_t idx) {
    m_parent->setCurrent(idx);
}

void CComboboxClickable::paint() {
    ;
}

void CComboboxClickable::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CComboboxClickable::size() {
    return impl->position.size();
}

std::optional<Vector2D> CComboboxClickable::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CComboboxClickable::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

std::optional<Vector2D> CComboboxClickable::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_layout->preferredSize(parent);
}

bool CComboboxClickable::acceptsMouseInput() {
    return true;
}

ePointerShape CComboboxClickable::pointerShape() {
    return HT_POINTER_ARROW;
}
