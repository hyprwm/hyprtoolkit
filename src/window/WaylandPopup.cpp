#include "WaylandPopup.hpp"
#include "WaylandWindow.hpp"

#include <hyprtoolkit/element/Null.hpp>

#include "../element/Element.hpp"
#include "../core/platforms/WaylandPlatform.hpp"
#include "../core/InternalBackend.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/AnimationManager.hpp"
#include "../layout/Positioner.hpp"

#include "../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CWaylandPopup::CWaylandPopup(const SPopupCreationData& data, SP<CWaylandWindow> window) : m_parent(window), m_creationData(data) {
    m_rootElement = CNullBuilder::begin()->commence();
}

CWaylandPopup::~CWaylandPopup() {
    close();
}

void CWaylandPopup::open() {
    if (m_open)
        return;

    m_rootElement->impl->window = m_self;
    m_rootElement->impl->breadthfirst([this](SP<IElement> e) { e->impl->window = m_self; });

    m_open = true;

    m_parent->updateFocus(Vector2D{-100000, -100000});

    m_waylandState.surface = makeShared<CCWlSurface>(g_waylandPlatform->m_waylandState.compositor->sendCreateSurface());

    if (!m_waylandState.surface->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no surface given. Errno: {}", errno);
        return;
    }

    m_waylandState.xdgSurface = makeShared<CCXdgSurface>(g_waylandPlatform->m_waylandState.xdg->sendGetXdgSurface(m_waylandState.surface->resource()));

    if (!m_waylandState.xdgSurface->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no xdgSurface given. Errno: {}", errno);
        return;
    }

    m_waylandState.xdgSurface->setConfigure([this](CCXdgSurface* r, uint32_t serial) {
        g_logger->log(HT_LOG_DEBUG, "wayland window: configure surface with {}", serial);
        r->sendAckConfigure(serial);
        m_waylandState.serial = serial;
    });

    m_wlPopupState.xdgPositioner = makeShared<CCXdgPositioner>(g_waylandPlatform->m_waylandState.xdg->sendCreatePositioner());

    m_wlPopupState.xdgPositioner->sendSetAnchorRect(m_creationData.pos.x, m_creationData.pos.y, 1, 1);
    m_wlPopupState.xdgPositioner->sendSetAnchor(XDG_POSITIONER_ANCHOR_TOP_LEFT);
    m_wlPopupState.xdgPositioner->sendSetGravity(XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
    m_wlPopupState.xdgPositioner->sendSetSize(m_creationData.size.x, m_creationData.size.y);
    m_wlPopupState.xdgPositioner->sendSetConstraintAdjustment(
        (xdgPositionerConstraintAdjustment)(XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X));

    m_wlPopupState.xdgPopup = makeShared<CCXdgPopup>(m_waylandState.xdgSurface->sendGetPopup(m_parent->m_waylandState.xdgSurface.get(), m_wlPopupState.xdgPositioner.get()));

    if (!m_wlPopupState.xdgPopup->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no xdgToplevel given. Errno: {}", errno);
        return;
    }

    m_wlPopupState.xdgPopup->setConfigure([this](CCXdgPopup* r, int32_t x, int32_t y, int32_t w, int32_t h) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure toplevel with {}x{}", w, h);

        if (!m_wlPopupState.grabbed) {
            m_wlPopupState.grabbed = true;
            m_wlPopupState.xdgPopup->sendGrab(g_waylandPlatform->m_waylandState.seat->resource(), g_waylandPlatform->m_lastEnterSerial);
        }

        if (m_waylandState.logicalSize == Vector2D{w, h})
            return;

        configure({w, h}, m_waylandState.serial);

        m_events.resized.emit(m_waylandState.logicalSize);
    });

    m_wlPopupState.xdgPopup->setPopupDone([this](CCXdgPopup* r) { close(); });

    m_waylandState.fractional = makeShared<CCWpFractionalScaleV1>(g_waylandPlatform->m_waylandState.fractional->sendGetFractionalScale(m_waylandState.surface->resource()));

    m_waylandState.fractional->setPreferredScale([this](CCWpFractionalScaleV1*, uint32_t scale) {
        const bool SAMESCALE = m_fractionalScale == scale / 120.0;
        m_fractionalScale    = scale / 120.0;

        g_logger->log(HT_LOG_DEBUG, "window: got fractional scale: {:.1f}%", m_fractionalScale * 100.F);

        if (!SAMESCALE)
            onScaleUpdate();
    });

    m_waylandState.viewport = makeShared<CCWpViewport>(g_waylandPlatform->m_waylandState.viewporter->sendGetViewport(m_waylandState.surface->resource()));

    auto inputRegion = makeShared<CCWlRegion>(g_waylandPlatform->m_waylandState.compositor->sendCreateRegion());
    inputRegion->sendAdd(0, 0, INT32_MAX, INT32_MAX);

    m_waylandState.surface->sendSetInputRegion(inputRegion.get());
    m_waylandState.surface->sendAttach(nullptr, 0, 0);
    m_waylandState.surface->sendCommit();

    inputRegion->sendDestroy();
}

void CWaylandPopup::close() {
    if (g_waylandPlatform->m_currentWindow == m_self)
        g_waylandPlatform->m_currentWindow = m_parent;

    if (m_parent)
        std::erase(m_parent->m_popups, m_self);

    if (!m_open)
        return;

    m_open = false;

    m_waylandState.frameCallback.reset();

    if (m_wlPopupState.xdgPopup)
        m_wlPopupState.xdgPopup->sendDestroy();
    if (m_waylandState.xdgSurface)
        m_waylandState.xdgSurface->sendDestroy();
    if (m_waylandState.surface)
        m_waylandState.surface->sendDestroy();

    m_waylandState = {};

    m_events.popupClosed.emit();
}

SP<IWindow> CWaylandPopup::openPopup(const SPopupCreationData& data) {
    return nullptr; //FIXME:
}
