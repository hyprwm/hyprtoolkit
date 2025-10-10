#include "Combobox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>
#include <hyprtoolkit/element/ScrollArea.hpp>
#include <hyprtoolkit/element/RowLayout.hpp>
#include <hyprtoolkit/element/ColumnLayout.hpp>
#include <hyprtoolkit/element/Button.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../../window/Window.hpp"
#include "../Element.hpp"

#include "../../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprgraphics;

SP<CComboboxElement> CComboboxElement::create(const SComboboxData& data) {
    auto p          = SP<CComboboxElement>(new CComboboxElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    p->init();
    return p;
}

constexpr float INNER_MARG             = 4.F;
constexpr float HANDLE_SIZE            = 12.F;
constexpr float DROPDOWN_BUTTON_HEIGHT = 30.F;
constexpr float DROPDOWN_BUTTON_PAD    = 2.F;
constexpr float DROPDOWN_MARGIN_OUTER  = 6.F;

CComboboxElement::CComboboxElement(const SComboboxData& data) : IElement(), m_impl(makeUnique<SComboboxImpl>()) {
    m_impl->data = data;
}

void CComboboxElement::init() {
    m_impl->layout = CRowLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1, 24}})->commence();

    m_impl->layout->setPositionFlag(HT_POSITION_FLAG_CENTER, true);
    m_impl->layout->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->layout->setMargin(INNER_MARG);

    m_impl->label = CTextBuilder::begin()
                        ->text(std::string{m_impl->data.items.at(m_impl->data.currentItem)})
                        ->color([] { return g_palette->m_colors.text; })
                        ->callback([this] {
                            if (impl->window)
                                impl->window->scheduleReposition(impl->self);
                        })
                        ->commence();

    m_impl->background = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.base; })
                             ->rounding(4)
                             ->borderColor([] { return g_palette->m_colors.alternateBase; })
                             ->borderThickness(1)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                             ->commence();

    m_impl->background->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->background->setPositionFlag(HT_POSITION_FLAG_CENTER, true);
    m_impl->background->impl->clipChildren = true;

    m_impl->handle = CDropdownHandleElement::create(SDropdownHandleData{
        .size  = {CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {HANDLE_SIZE, HANDLE_SIZE}},
        .color = [] { return g_palette->m_colors.text; },
    });

    m_impl->leftPad   = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {INNER_MARG, 1.F}})->commence();
    m_impl->rightPad  = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {INNER_MARG, 1.F}})->commence();
    m_impl->middlePad = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {INNER_MARG, 1.F}})->commence();

    m_impl->middlePad->setGrow(true);

    m_impl->layout->addChild(m_impl->leftPad);
    m_impl->layout->addChild(m_impl->label);
    m_impl->layout->addChild(m_impl->middlePad);
    m_impl->layout->addChild(m_impl->handle);
    m_impl->layout->addChild(m_impl->rightPad);

    addChild(m_impl->background);
    addChild(m_impl->layout);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_impl->primedForUp = false;
        m_impl->background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })
            ->commence();
    });
    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_impl->primedForUp = false;
        m_impl->background
            ->rebuild() //
            ->borderColor([] { return g_palette->m_colors.alternateBase; })
            ->commence();
    });

    m_impl->listeners.clicked = impl->m_externalEvents.mouseButton.listen([this](Input::eMouseButton button, bool down) {
        if (button != Input::MOUSE_BUTTON_LEFT)
            return;

        if (down) {
            m_impl->primedForUp = true;
            return;
        }

        if (!m_impl->primedForUp)
            return;

        openDropdown();
    });
}

void CComboboxElement::openDropdown() {
    if (m_impl->dropdown.popup)
        return;

    const Vector2D POPUP_SIZE = Vector2D{
        100.F + impl->position.size().x + (2 * DROPDOWN_BUTTON_PAD),
        std::min( //
            (DROPDOWN_BUTTON_HEIGHT * m_impl->data.items.size()) + (DROPDOWN_BUTTON_PAD * (m_impl->data.items.size() - 1)) + (2 * DROPDOWN_BUTTON_PAD) +
                (2 * DROPDOWN_MARGIN_OUTER), //
            600.F                            //
            ),
    };

    m_impl->dropdown.popup = impl->window->openPopup(SWindowCreationData{
        .preferredSize = POPUP_SIZE,
        .pos           = impl->position.pos() - Vector2D{50.F, 0.F} + Vector2D{0.F, impl->position.size().y},
    });

    m_impl->listeners.popupClosed = m_impl->dropdown.popup->m_events.popupClosed.listen([this, self = m_impl->self] {
        if (!self)
            return;

        m_impl->dropdown = {};
    });

    m_impl->dropdown.scroll = CScrollAreaBuilder::begin()->scrollY(true)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})->commence();

    m_impl->dropdown.layout = CColumnLayoutBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->gap(DROPDOWN_BUTTON_PAD)->commence();
    m_impl->dropdown.layout->setMargin(DROPDOWN_BUTTON_PAD);

    m_impl->dropdown.background = CRectangleBuilder::begin()
                                      ->color([] { return g_palette->m_colors.background; })
                                      ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}})
                                      ->rounding(10)
                                      ->borderColor([] { return g_palette->m_colors.alternateBase; })
                                      ->borderThickness(1)
                                      ->commence();

    m_impl->dropdown.background->setMargin(1);
    m_impl->dropdown.scroll->setMargin(DROPDOWN_MARGIN_OUTER);

    m_impl->dropdown.popup->m_rootElement->addChild(m_impl->dropdown.background);
    m_impl->dropdown.background->addChild(m_impl->dropdown.scroll);
    m_impl->dropdown.scroll->addChild(m_impl->dropdown.layout);

    m_impl->dropdown.buttons.clear();

    for (size_t i = 0; i < m_impl->data.items.size(); ++i) {
        m_impl->dropdown.buttons.emplace_back(CButtonBuilder::begin()
                                                  ->label(std::string{m_impl->data.items.at(i)})
                                                  ->noBorder(true)
                                                  ->noBg(true)
                                                  ->alignText(HT_FONT_ALIGN_LEFT)
                                                  ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, DROPDOWN_BUTTON_HEIGHT}})
                                                  ->onMainClick([i, this, self = m_impl->self](SP<CButtonElement> e) {
                                                      if (!self)
                                                          return;

                                                      setSelection(i);
                                                      if (m_impl->data.onChanged)
                                                          m_impl->data.onChanged(m_impl->self.lock(), i);

                                                      g_backend->addIdle([this, self = m_impl->self] {
                                                          if (!self)
                                                              return;

                                                          closeDropdown();
                                                      });
                                                  })
                                                  ->commence() //
        );

        m_impl->dropdown.layout->addChild(m_impl->dropdown.buttons.back());
    }

    m_impl->dropdown.popup->open();

    impl->grouped = true;
}

void CComboboxElement::closeDropdown() {
    if (!m_impl->dropdown.popup)
        return;

    m_impl->dropdown.popup->close();
}

void CComboboxElement::setSelection(size_t idx) {
    setCurrent(idx);
}

size_t CComboboxElement::current() {
    return m_impl->data.currentItem;
}

void CComboboxElement::setCurrent(size_t current) {
    m_impl->data.currentItem = std::min(current, m_impl->data.items.size() - 1);

    m_impl->label
        ->rebuild() //
        ->text(std::string{m_impl->data.items.at(m_impl->data.currentItem)})
        ->commence();
}

void CComboboxElement::replaceData(const SComboboxData& data) {
    m_impl->data = data;

    setCurrent(data.currentItem);

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CComboboxElement::paint() {
    ;
}

SP<CComboboxBuilder> CComboboxElement::rebuild() {
    auto p       = SP<CComboboxBuilder>(new CComboboxBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SComboboxData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CComboboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

Hyprutils::Math::Vector2D CComboboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CComboboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CComboboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

std::optional<Vector2D> CComboboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return m_impl->data.size.calculate(parent);
}

bool CComboboxElement::acceptsMouseInput() {
    return true;
}

ePointerShape CComboboxElement::pointerShape() {
    return HT_POINTER_ARROW;
}

bool CComboboxElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
