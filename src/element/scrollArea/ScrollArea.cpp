#include "ScrollArea.hpp"
#include <cmath>
#include <algorithm>

#include <hyprtoolkit/element/Null.hpp>
#include <hyprtoolkit/element/Rectangle.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include "../../layout/Positioner.hpp"
#include "../../core/InternalBackend.hpp"
#include "../../window/ToolkitWindow.hpp"

#include "../Element.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

constexpr float SCROLLBAR_W   = 6.F;  // thickness of the bar
constexpr float SCROLLBAR_PAD = 2.F;  // gap at each end of the track
constexpr float SCROLLBAR_MIN = 24.F; // shortest the thumb is allowed to get
constexpr float THUMB_IDLE_A  = 0.4F;
constexpr float THUMB_HOT_A   = 0.7F;
constexpr float TRACK_A       = 0.18F;

// thumb gets brighter while hovered or dragged
static void refreshThumbColor(SScrollbar& bar) {
    if (!bar.thumb)
        return;
    const bool hot = bar.hovered || bar.dragging;
    bar.thumb->rebuild()
        ->color([hot] {
            auto c = g_palette->m_colors.text;
            c.a    = hot ? THUMB_HOT_A : THUMB_IDLE_A;
            return c;
        })
        ->commence();
}

// map a pointer position along the track (strip-local, main axis) to a scroll offset
static void scrollFromThumb(CScrollAreaElement& area, SScrollbar& bar, bool vertical, float mainAxisLocal) {
    const float span = bar.trackLen - bar.thumbLen;
    if (span <= 0.F || bar.maxOff <= 0.F)
        return;

    const float thumbOff = std::clamp(mainAxisLocal - bar.grab, SCROLLBAR_PAD, SCROLLBAR_PAD + span);
    const float frac     = (thumbOff - SCROLLBAR_PAD) / span;

    auto        scroll = area.getCurrentScroll();
    if (vertical)
        scroll.y = frac * bar.maxOff;
    else
        scroll.x = frac * bar.maxOff;

    area.setScroll(scroll);
}

SP<CScrollAreaElement> CScrollAreaElement::create(const SScrollAreaData& data) {
    auto p          = SP<CScrollAreaElement>(new CScrollAreaElement(data));
    p->impl->self   = p;
    p->m_impl->self = p;
    p->init();
    return p;
}

CScrollAreaElement::CScrollAreaElement(const SScrollAreaData& data) : IElement(), m_impl(makeUnique<SScrollAreaImpl>()) {
    m_impl->data       = data;
    impl->clipChildren = true;

    m_impl->listeners.axis = impl->m_externalEvents.mouseAxis.listen([this](Input::eAxisAxis axis, float delta) {
        if (m_impl->data.blockUserScroll)
            return;

        const bool SCROLLING_X = axis == Input::AXIS_AXIS_HORIZONTAL;

        if (SCROLLING_X && !m_impl->data.scrollX)
            return;
        if (!SCROLLING_X && !m_impl->data.scrollY)
            return;

        if (SCROLLING_X)
            m_impl->data.currentScroll.x += delta;
        else
            m_impl->data.currentScroll.y += delta;

        m_impl->clampMaxScroll();

        if (impl->window)
            impl->window->scheduleReposition(impl->self);
    });
}

void CScrollAreaElement::init() {
    m_impl->inner = CNullBuilder::begin()->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})->commence();
    // the scroll offset rides on the inner layer's own position (an absolute offset), not a
    // one-shot pass over its children. so any reposition of inner, including a bare one the
    // window does when a content child schedules a reflow, keeps the content scrolled instead
    // of snapping it back to the top.
    m_impl->inner->setPositionMode(HT_POSITION_ABSOLUTE);
    IElement::addChild(m_impl->inner);

    rebuildScrollbars();
}

void CScrollAreaElement::rebuildScrollbars() {
    const auto buildOne = [this](SScrollbar& bar, bool vertical) {
        bar = SScrollbar{};

        bar.strip = CNullBuilder::begin()
                        ->size(vertical ? CDynamicSize{CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_PERCENT, {SCROLLBAR_W, 1.F}} :
                                          CDynamicSize{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_ABSOLUTE, {1.F, SCROLLBAR_W}})
                        ->commence();
        bar.strip->setPositionMode(HT_POSITION_ABSOLUTE);
        bar.strip->setPositionFlag(vertical ? HT_POSITION_FLAG_RIGHT : HT_POSITION_FLAG_BOTTOM, true);
        bar.strip->setReceivesMouse(true);

        bar.track = CRectangleBuilder::begin()
                        ->color([] {
                            auto c = g_palette->m_colors.alternateBase;
                            c.a    = TRACK_A;
                            return c;
                        })
                        ->rounding(static_cast<int>(SCROLLBAR_W / 2.F))
                        ->size({CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1.F, 1.F}})
                        ->commence();

        bar.thumb = CRectangleBuilder::begin()
                        ->color([] {
                            auto c = g_palette->m_colors.text;
                            c.a    = THUMB_IDLE_A;
                            return c;
                        })
                        ->rounding(static_cast<int>(SCROLLBAR_W / 2.F))
                        ->size({CDynamicSize::HT_SIZE_ABSOLUTE, CDynamicSize::HT_SIZE_ABSOLUTE, {SCROLLBAR_W, SCROLLBAR_W}})
                        ->commence();
        bar.thumb->setPositionMode(HT_POSITION_ABSOLUTE);

        bar.strip->addChild(bar.track);
        bar.strip->addChild(bar.thumb);

        bar.listeners.move = bar.strip->impl->m_externalEvents.mouseMove.listen([this, &bar, vertical](const Vector2D& local) {
            bar.lastLocal = local;
            if (bar.dragging)
                scrollFromThumb(*this, bar, vertical, vertical ? local.y : local.x);
        });

        bar.listeners.button = bar.strip->impl->m_externalEvents.mouseButton.listen([this, &bar, vertical](Input::eMouseButton button, bool down) {
            if (button != Input::MOUSE_BUTTON_LEFT)
                return;

            if (!down) {
                bar.dragging = false;
                refreshThumbColor(bar);
                return;
            }

            const float pos = vertical ? bar.lastLocal.y : bar.lastLocal.x;
            if (pos >= bar.thumbOff && pos < bar.thumbOff + bar.thumbLen)
                bar.grab = pos - bar.thumbOff; // grabbed the thumb where the cursor landed
            else
                bar.grab = bar.thumbLen / 2.F; // clicked the empty track, jump so the thumb centers under the cursor

            bar.dragging = true;
            refreshThumbColor(bar);
            scrollFromThumb(*this, bar, vertical, pos);
        });

        bar.listeners.enter = bar.strip->impl->m_externalEvents.mouseEnter.listen([&bar](const Vector2D& local) {
            bar.lastLocal = local;
            bar.hovered   = true;
            refreshThumbColor(bar);
        });

        bar.listeners.leave = bar.strip->impl->m_externalEvents.mouseLeave.listen([&bar] {
            bar.hovered = false;
            refreshThumbColor(bar);
        });
    };

    if (m_impl->data.showScrollbar) {
        buildOne(m_impl->vbar, true);
        buildOne(m_impl->hbar, false);
    } else {
        if (m_impl->vbar.strip)
            IElement::removeChild(m_impl->vbar.strip);
        if (m_impl->hbar.strip)
            IElement::removeChild(m_impl->hbar.strip);
        m_impl->vbar = SScrollbar{};
        m_impl->hbar = SScrollbar{};
    }
}

void CScrollAreaElement::layoutScrollbars() {
    const auto layoutOne = [this](SScrollbar& bar, bool vertical) {
        if (!bar.strip)
            return;

        const auto  max    = m_impl->maxScroll();
        const float maxOff = vertical ? max.y : max.x;
        const bool  axisOn = vertical ? m_impl->data.scrollY : m_impl->data.scrollX;
        const bool  want   = axisOn && maxOff > 0.F;

        if (want && !bar.shown) {
            IElement::addChild(bar.strip);
            bar.shown = true;
        } else if (!want && bar.shown) {
            IElement::removeChild(bar.strip);
            bar.shown = false;
        }

        if (!want)
            return;

        const CBox     view    = impl->position;
        const Vector2D content = view.size() + max;

        const float    viewLen = vertical ? view.h : view.w;
        const float    contLen = vertical ? content.y : content.x;
        const float    scroll  = vertical ? m_impl->data.currentScroll.y : m_impl->data.currentScroll.x;

        bar.trackLen = viewLen - SCROLLBAR_PAD * 2.F;
        if (bar.trackLen <= 0.F)
            return; // viewport too small to draw a bar

        bar.maxOff = maxOff;
        // thumb wants to be the visible fraction of the track, but never shorter than the min,
        // and never longer than the track itself (std::min/max, not clamp, so the order is always valid)
        bar.thumbLen = std::min(bar.trackLen, std::max(SCROLLBAR_MIN, bar.trackLen * (viewLen / contLen)));
        bar.thumbOff = SCROLLBAR_PAD + (scroll / maxOff) * (bar.trackLen - bar.thumbLen);

        CBox stripBox;
        CBox thumbBox;
        if (vertical) {
            stripBox = {view.x + view.w - SCROLLBAR_W, view.y, SCROLLBAR_W, view.h};
            thumbBox = {stripBox.x, stripBox.y + bar.thumbOff, SCROLLBAR_W, bar.thumbLen};
        } else {
            stripBox = {view.x, view.y + view.h - SCROLLBAR_W, view.w, SCROLLBAR_W};
            thumbBox = {stripBox.x + bar.thumbOff, stripBox.y, bar.thumbLen, SCROLLBAR_W};
        }

        g_positioner->position(bar.strip, stripBox); // also lays out the track (fills the strip)
        g_positioner->position(bar.thumb, thumbBox);
    };

    layoutOne(m_impl->vbar, true);
    layoutOne(m_impl->hbar, false);
}

void CScrollAreaElement::addChild(SP<IElement> child) {
    m_impl->inner->addChild(child);
}

void CScrollAreaElement::removeChild(SP<IElement> child) {
    m_impl->inner->removeChild(child);
}

void CScrollAreaElement::clearChildren() {
    m_impl->inner->clearChildren();
}

void CScrollAreaElement::paint() {
    ; // no-op, the children draw themselves
}

void CScrollAreaElement::replaceData(const SScrollAreaData& data) {
    const bool barChanged = m_impl->data.showScrollbar != data.showScrollbar;
    m_impl->data          = data;

    if (barChanged)
        rebuildScrollbars();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

void CScrollAreaElement::reposition(const Hyprutils::Math::CBox& sbox, const Vector2D& maxSize) {
    IElement::reposition(sbox);

    m_impl->clampMaxScroll();

    // move the whole content layer by the scroll offset, then lay it (and the scrollbar
    // strips) out. the strips carry no offset of their own, so they stay pinned to the edge.
    m_impl->inner->setAbsolutePosition(-m_impl->data.currentScroll.round());
    g_positioner->positionChildren(impl->self.lock());

    // content is laid out at its natural size now, so this reads the real overflow.
    m_impl->recalcMaxScroll();

    if (m_impl->data.showScrollbar)
        layoutScrollbars();
}

Vector2D CScrollAreaElement::size() {
    return impl->position.size();
}

Vector2D CScrollAreaElement::getCurrentScroll() {
    return m_impl->data.currentScroll;
}

void CScrollAreaElement::setScroll(const Hyprutils::Math::Vector2D& x) {
    m_impl->data.currentScroll = x;
    m_impl->clampMaxScroll();

    if (impl->window)
        impl->window->scheduleReposition(impl->self);
}

std::optional<Vector2D> CScrollAreaElement::preferredSize(const Vector2D& parent) {
    return impl->getPreferredSizeGeneric(m_impl->data.size, parent);
}

std::optional<Vector2D> CScrollAreaElement::minimumSize(const Vector2D& parent) {
    return Vector2D{0, 0};
}

bool CScrollAreaElement::acceptsMouseInput() {
    return true;
}

ePointerShape CScrollAreaElement::pointerShape() {
    return HT_POINTER_ARROW;
}

bool CScrollAreaElement::alwaysGetMouseInput() {
    return true;
}

bool CScrollAreaElement::positioningDependsOnChild() {
    return false;
}

Vector2D SScrollAreaImpl::maxScroll() {
    return cachedMaxScroll;
}

void SScrollAreaImpl::recalcMaxScroll() {
    if (inner->impl->children.empty() || !inner->impl->children.at(0)->impl->positionerData) {
        cachedMaxScroll = {0, 0};
        return;
    }

    cachedMaxScroll = (inner->impl->children.at(0)
                           ->preferredSize({
                               data.scrollX ? 99999999999 : self->impl->position.w,
                               data.scrollY ? 99999999999 : self->impl->position.h,
                           })
                           .value_or(Vector2D{99999999, 99999999}) -
                       self->impl->position.size())
                          .clamp({0, 0});
}

void SScrollAreaImpl::clampMaxScroll() {
    // recheck limits
    if (inner->impl->children.empty() || !inner->impl->children.at(0)->impl->positionerData)
        return;

    data.currentScroll = data.currentScroll.clamp({}, maxScroll());
}
