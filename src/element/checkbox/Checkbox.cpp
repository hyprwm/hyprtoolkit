#include "Checkbox.hpp"

#include <hyprtoolkit/palette/Palette.hpp>

#include "../../core/InternalBackend.hpp"
#include "../../layout/Positioner.hpp"
#include "../../renderer/Renderer.hpp"
#include "../../window/ToolkitWindow.hpp"
#include "../../core/AnimationManager.hpp"
#include "../Element.hpp"

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

    m_impl->background = CRectangleBuilder::begin()
                             ->color([] { return g_palette->m_colors.base; })
                             ->rounding(4)
                             ->borderColor([] { return g_palette->m_colors.alternateBase; })
                             ->borderThickness(1)
                             ->size(CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {14.F, 14.F}})
                             ->commence();

    m_impl->background->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->background->setPositionFlag(HT_POSITION_FLAG_CENTER, true);

    CHyprColor col = g_palette->m_colors.accent;
    col.a          = m_impl->data.toggled ? 1.F : 0.F;
    m_impl->foreground =
        CCheckmarkElement::create(SCheckmarkData{.size = {CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}}, .color = [col] { return col; }});

    m_impl->foreground->setPositionMode(HT_POSITION_ABSOLUTE);
    m_impl->foreground->setPositionFlag(HT_POSITION_FLAG_CENTER, true);

    m_impl->background->addChild(m_impl->foreground);

    addChild(m_impl->background);

    impl->m_externalEvents.mouseEnter.listenStatic([this](const Vector2D& pos) {
        m_impl->background
            ->rebuild() //
            ->color([] { return g_palette->m_colors.base.brighten(0.11F); })
            ->borderColor([] { return g_palette->m_colors.alternateBase.brighten(0.5F); })
            ->commence();
        m_impl->primedForUp = false;
    });

    impl->m_externalEvents.mouseLeave.listenStatic([this]() {
        m_impl->background
            ->rebuild() //
            ->color([] { return g_palette->m_colors.base; })
            ->borderColor([] { return g_palette->m_colors.alternateBase; })
            ->commence();
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

            CHyprColor col               = g_palette->m_colors.accent;
            col.a                        = m_impl->data.toggled ? 1.F : 0.F;
            *m_impl->foreground->m_color = col;
        }
    });

    impl->grouped = true;
}

void CCheckboxElement::paint() {
    ;
}

void CCheckboxElement::reposition(const Hyprutils::Math::CBox& box, const Hyprutils::Math::Vector2D& maxSize) {
    IElement::reposition(box);

    g_positioner->positionChildren(impl->self.lock());
}

SP<CCheckboxBuilder> CCheckboxElement::rebuild() {
    auto p       = SP<CCheckboxBuilder>(new CCheckboxBuilder());
    p->m_self    = p;
    p->m_data    = makeUnique<SCheckboxData>(m_impl->data);
    p->m_element = m_impl->self;
    return p;
}

void CCheckboxElement::replaceData(const SCheckboxData& data) {
    m_impl->data = data;

    CHyprColor col               = g_palette->m_colors.accent;
    col.a                        = m_impl->data.toggled ? 1.F : 0.F;
    *m_impl->foreground->m_color = col;

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

Hyprutils::Math::Vector2D CCheckboxElement::size() {
    return impl->position.size();
}

std::optional<Vector2D> CCheckboxElement::preferredSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CCheckboxElement::minimumSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CCheckboxElement::maximumSize(const Hyprutils::Math::Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

bool CCheckboxElement::acceptsMouseInput() {
    return true;
}

ePointerShape CCheckboxElement::pointerShape() {
    return HT_POINTER_POINTER;
}

bool CCheckboxElement::positioningDependsOnChild() {
    return m_impl->data.size.hasAuto();
}
