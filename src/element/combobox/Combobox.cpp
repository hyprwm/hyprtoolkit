#include "Combobox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
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

CComboboxElement::CComboboxElement(const SComboboxData& data) : IElement(), m_impl(makeUnique<SComboboxImpl>()) {
    m_impl->data = data;
}

void CComboboxElement::init() {
    RASSERT(!m_impl->data.items.empty(), "Spinbox can't be empty");

    m_impl->layout =
        CRowLayoutBuilder::begin()->gap(3)->size({m_impl->data.fill ? CDynamicSize::HT_SIZE_PERCENT : CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})->commence();

    m_impl->label = CTextBuilder::begin()
                        ->text(std::string{m_impl->data.label})
                        ->color([] { return g_palette->m_colors.text; })
                        ->size({CDynamicSize::HT_SIZE_AUTO, CDynamicSize::HT_SIZE_AUTO, {1, 1}})
                        ->commence();

    m_impl->clickable = CComboboxClickable::create(m_impl->self.lock());

    m_impl->spacer = CNullBuilder::begin()->commence();
    m_impl->spacer->setGrow(true, true);

    m_impl->layout->addChild(m_impl->label);
    m_impl->layout->addChild(m_impl->spacer);
    m_impl->layout->addChild(m_impl->clickable);

    addChild(m_impl->layout);
}

void CComboboxElement::paint() {
    ;
}

void CComboboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

void CComboboxElement::replaceData(const SComboboxData& data) {
    m_impl->data = data;

    m_impl->label->rebuild()->text(std::string{data.label})->commence();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

SP<CComboboxBuilder> CComboboxElement::rebuild() {
    auto p       = SP<CComboboxBuilder>(new CComboboxBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SComboboxData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

size_t CComboboxElement::current() {
    return m_impl->data.currentItem;
}

void CComboboxElement::setCurrent(size_t current) {
    m_impl->data.currentItem = std::min(current, m_impl->data.items.size() - 1);

    m_impl->clickable->m_label
        ->rebuild() //
        ->text(std::string{m_impl->data.items.at(m_impl->data.currentItem)})
        ->commence();
}

Hyprutils::Math::Vector2D CComboboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CComboboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CComboboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
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

std::optional<Vector2D> CComboboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
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

bool CComboboxElement::acceptsMouseInput() {
    return false;
}

ePointerShape CComboboxElement::pointerShape() {
    return HT_POINTER_POINTER;
}
