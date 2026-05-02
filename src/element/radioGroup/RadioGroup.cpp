#include "RadioGroup.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

static constexpr float DOT_OUTER = 14.F;
static constexpr float DOT_INNER = 8.F;

SP<CRadioGroupElement> CRadioGroupElement::create(const SRadioGroupData& data) {
    auto p          = SP<CRadioGroupElement>(new CRadioGroupElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    return p;
}

CRadioGroupElement::CRadioGroupElement(const SRadioGroupData& data) : IElement(), m_impl(makeUnique<SRadioGroupImpl>()) {
    m_impl->data = data;

    m_impl->layout = CColumnLayoutBuilder::begin()->gap(m_impl->data.gap)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.F, 1.F}})->commence();

    for (size_t i = 0; i < m_impl->data.items.size(); ++i) {
        auto outer = CRectangleBuilder::begin()
                         ->color([] { return g_palette->m_colors.base; })
                         ->rounding(sc<int>(DOT_OUTER))
                         ->borderColor([] { return g_palette->m_colors.alternateBase; })
                         ->borderThickness(1)
                         ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {DOT_OUTER, DOT_OUTER}})
                         ->commence();

        CHyprColor col = g_palette->m_colors.accent;
        col.a          = (sc<int>(i) == m_impl->data.selected) ? 1.F : 0.F;

        auto inner = CRectangleBuilder::begin()
                         ->color([col] { return col; })
                         ->rounding(sc<int>(DOT_INNER))
                         ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {DOT_INNER, DOT_INNER}})
                         ->commence();

        inner->setPositionMode(HT_POSITION_ABSOLUTE);
        inner->setPositionFlag(HT_POSITION_FLAG_CENTER, true);
        outer->addChild(inner);

        auto label = CTextBuilder::begin() //
                         ->text(std::string{m_impl->data.items[i]})
                         ->color([] { return g_palette->m_colors.text; })
                         ->commence();

        auto row = CRowLayoutBuilder::begin()->gap(6)->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_AUTO, {1.F, 1.F}})->commence();
        row->addChild(outer);
        row->addChild(label);

        const int IDX = sc<int>(i);
        row->impl->m_externalEvents.mouseButton.listenStatic([this, IDX](const Input::eMouseButton button, bool down) {
            if (down || button != Input::MOUSE_BUTTON_LEFT)
                return;
            m_impl->select(IDX);
        });
        row->impl->userRequestedMouseInput = true;

        m_impl->dots.emplace_back(inner);
        m_impl->rows.emplace_back(row);

        m_impl->layout->addChild(row);
    }

    addChild(m_impl->layout);

    impl->grouped = true;
}

void SRadioGroupImpl::applySelection() {
    for (size_t i = 0; i < dots.size(); ++i) {
        CHyprColor col = g_palette->m_colors.accent;
        col.a          = (sc<int>(i) == data.selected) ? 1.F : 0.F;
        dots[i]->rebuild()->color([col] { return col; })->commence();
    }
}

void SRadioGroupImpl::select(int idx) {
    if (idx == data.selected || idx < 0 || idx >= sc<int>(data.items.size()))
        return;

    data.selected = idx;
    applySelection();

    if (data.onSelected)
        data.onSelected(self.lock(), idx);
}

void CRadioGroupElement::paint() {
    ;
}

void CRadioGroupElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);
    g_positioner->positionChildren(impl->self.lock());
}

SP<CRadioGroupBuilder> CRadioGroupElement::rebuild() {
    auto p       = SP<CRadioGroupBuilder>(new CRadioGroupBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SRadioGroupData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CRadioGroupElement::replaceData(const SRadioGroupData& data) {
    const bool SEL_CHANGED = data.selected != m_impl->data.selected;
    m_impl->data           = data;

    if (SEL_CHANGED)
        m_impl->applySelection();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

int CRadioGroupElement::selectedIndex() {
    return m_impl->data.selected;
}

void CRadioGroupElement::setSelected(int idx) {
    m_impl->select(idx);
}

Hyprutils::Math::Vector2D CRadioGroupElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CRadioGroupElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CRadioGroupElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CRadioGroupElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

bool CRadioGroupElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
