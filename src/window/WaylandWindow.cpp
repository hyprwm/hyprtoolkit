#include "WaylandWindow.hpp"
#include "WaylandPopup.hpp"

#include <hyprtoolkit/element/Null.hpp>

#include "../element/Element.hpp"
#include "../core/platforms/WaylandPlatform.hpp"
#include "../core/InternalBackend.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/AnimationManager.hpp"

#include "../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CWaylandWindow::CWaylandWindow(const SWindowCreationData& data) : m_creationData(data) {
    m_rootElement = CNullBuilder::begin()->commence();
}

CWaylandWindow::~CWaylandWindow() {
    close();

    if (g_waylandPlatform)
        std::erase_if(g_waylandPlatform->m_windows, [this](const auto& e) { return e.get() == this; });
}

void CWaylandWindow::open() {
    if (m_open)
        return;

    m_open = true;

    m_rootElement->impl->window = m_self;
    m_rootElement->impl->breadthfirst([this](SP<IElement> e) { e->impl->window = m_self; });

    m_waylandState.surface = makeShared<CCWlSurface>(g_waylandPlatform->m_waylandState.compositor->sendCreateSurface());

    if (!m_waylandState.surface->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no surface given. Errno: {}", errno);
        return;
    }

    if (m_creationData.inhibitShortcuts && g_waylandPlatform->m_waylandState.shortcutsInhibitMgr && g_waylandPlatform->m_waylandState.seat)
        m_waylandState.shortcutsInhibitor = makeShared<CCZwpKeyboardShortcutsInhibitorV1>(
            g_waylandPlatform->m_waylandState.shortcutsInhibitMgr->sendInhibitShortcuts(m_waylandState.surface->resource(), g_waylandPlatform->m_waylandState.seat->resource()));

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

    m_waylandState.xdgToplevel = makeShared<CCXdgToplevel>(m_waylandState.xdgSurface->sendGetToplevel());

    if (!m_waylandState.xdgToplevel->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no xdgToplevel given. Errno: {}", errno);
        return;
    }

    m_waylandState.xdgToplevel->setWmCapabilities([](CCXdgToplevel* r, wl_array* arr) { g_logger->log(HT_LOG_DEBUG, "window opening failed: wm_capabilities received"); });

    m_waylandState.xdgToplevel->setConfigure([this](CCXdgToplevel* r, int32_t w, int32_t h, wl_array* arr) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure toplevel with {}x{}", w, h);

        m_isMaximized = m_isFullscreen = m_isResizing = m_isTiled = false;
        if (arr) {
            const auto  count  = arr->size / sizeof(uint32_t);
            const auto* states = static_cast<const uint32_t*>(arr->data);
            for (size_t i = 0; i < count; ++i) {
                switch (states[i]) {
                    case XDG_TOPLEVEL_STATE_MAXIMIZED: m_isMaximized = true; break;
                    case XDG_TOPLEVEL_STATE_FULLSCREEN: m_isFullscreen = true; break;
                    case XDG_TOPLEVEL_STATE_RESIZING: m_isResizing = true; break;
                    case XDG_TOPLEVEL_STATE_TILED_LEFT:
                    case XDG_TOPLEVEL_STATE_TILED_RIGHT:
                    case XDG_TOPLEVEL_STATE_TILED_TOP:
                    case XDG_TOPLEVEL_STATE_TILED_BOTTOM: m_isTiled = true; break;
                    default: break;
                }
            }
        }

        if (w == 0 || h == 0) {
            if (m_creationData.preferredSize) {
                w = m_creationData.preferredSize->x;
                h = m_creationData.preferredSize->y;
            } else {
                g_logger->log(HT_LOG_DEBUG, "configure: w/h is 0, sending default hardcoded 1280x720");
                w = 1280;
                h = 720;
            }
        }

        if (m_waylandState.logicalSize != Vector2D{w, h}) {
            configure({w, h}, m_waylandState.serial);
            m_events.resized.emit(m_waylandState.logicalSize);
        }

        // unpin the min == max lock only once the compositor's configure size
        // actually matches our requested size. animating compositors send
        // intermediate sizes between our request and the final target; relaxing
        // constraints on those would let the compositor re-target away.
        if (m_pendingResize.pending) {
            const auto dx = std::abs(static_cast<int>(w) - static_cast<int>(m_pendingResize.size.x));
            const auto dy = std::abs(static_cast<int>(h) - static_cast<int>(m_pendingResize.size.y));
            if (dx <= 1 && dy <= 1) {
                restoreUserSizeConstraints();
                m_waylandState.surface->sendCommit();
                m_pendingResize.pending = false;
            }
        }
    });

    m_waylandState.xdgToplevel->setClose([this](CCXdgToplevel* r) { m_events.closeRequest.emit(); });

    m_waylandState.fractional = makeShared<CCWpFractionalScaleV1>(g_waylandPlatform->m_waylandState.fractional->sendGetFractionalScale(m_waylandState.surface->resource()));

    m_waylandState.fractional->setPreferredScale([this](CCWpFractionalScaleV1*, uint32_t scale) {
        const bool SAMESCALE = m_fractionalScale == scale / 120.0;
        m_fractionalScale    = scale / 120.0;

        g_logger->log(HT_LOG_DEBUG, "window: got fractional scale: {:.1f}%", m_fractionalScale * 100.F);

        if (!SAMESCALE)
            onScaleUpdate();
    });

    m_waylandState.viewport = makeShared<CCWpViewport>(g_waylandPlatform->m_waylandState.viewporter->sendGetViewport(m_waylandState.surface->resource()));

    m_waylandState.xdgToplevel->sendSetTitle(m_creationData.title.c_str());
    m_waylandState.xdgToplevel->sendSetAppId(m_creationData.class_.c_str());

    if (m_creationData.minSize)
        m_waylandState.xdgToplevel->sendSetMinSize(m_creationData.minSize->x, m_creationData.minSize->y);
    if (m_creationData.maxSize)
        m_waylandState.xdgToplevel->sendSetMaxSize(m_creationData.maxSize->x, m_creationData.maxSize->y);

    auto inputRegion = makeShared<CCWlRegion>(g_waylandPlatform->m_waylandState.compositor->sendCreateRegion());
    inputRegion->sendAdd(0, 0, INT32_MAX, INT32_MAX);

    m_waylandState.surface->sendSetInputRegion(inputRegion.get());
    m_waylandState.surface->sendAttach(nullptr, 0, 0);
    m_waylandState.surface->sendCommit();

    inputRegion->sendDestroy();
}

void CWaylandWindow::close() {
    if (!m_open)
        return;

    m_open = false;

    m_waylandState.frameCallback.reset();

    if (m_waylandState.xdgToplevel)
        m_waylandState.xdgToplevel->sendDestroy();
    if (m_waylandState.xdgSurface)
        m_waylandState.xdgSurface->sendDestroy();
    if (m_waylandState.surface)
        m_waylandState.surface->sendDestroy();

    m_waylandState = {};
}

void CWaylandWindow::setSize(const Vector2D& size) {
    if (!m_open || !m_waylandState.xdgToplevel)
        return;
    if (size.x <= 0 || size.y <= 0)
        return;

    Vector2D clamped = size;
    if (m_creationData.minSize) {
        clamped.x = std::max(clamped.x, m_creationData.minSize->x);
        clamped.y = std::max(clamped.y, m_creationData.minSize->y);
    }
    if (m_creationData.maxSize) {
        clamped.x = std::min(clamped.x, m_creationData.maxSize->x);
        clamped.y = std::min(clamped.y, m_creationData.maxSize->y);
    }

    // xdg-shell has no client-side resize request. the standard workaround is to
    // pin min == max so the compositor's next configure must use this size, then
    // restore the user's constraints on that configure (see setConfigure handler).
    m_waylandState.xdgToplevel->sendSetMinSize(clamped.x, clamped.y);
    m_waylandState.xdgToplevel->sendSetMaxSize(clamped.x, clamped.y);
    m_waylandState.surface->sendCommit();

    m_pendingResize.pending = true;
    m_pendingResize.size    = clamped;
}

void CWaylandWindow::restoreUserSizeConstraints() {
    const auto minW = m_creationData.minSize ? m_creationData.minSize->x : 0;
    const auto minH = m_creationData.minSize ? m_creationData.minSize->y : 0;
    const auto maxW = m_creationData.maxSize ? m_creationData.maxSize->x : 0;
    const auto maxH = m_creationData.maxSize ? m_creationData.maxSize->y : 0;
    m_waylandState.xdgToplevel->sendSetMinSize(minW, minH);
    m_waylandState.xdgToplevel->sendSetMaxSize(maxW, maxH);
}

void CWaylandWindow::startInteractiveResize(eResizeEdge edges) {
    if (!m_open || !m_waylandState.xdgToplevel || edges == HT_RESIZE_EDGE_NONE)
        return;

    if (!g_waylandPlatform->m_waylandState.seat)
        return;

    m_waylandState.xdgToplevel->sendResize(g_waylandPlatform->m_waylandState.seat->resource(), g_waylandPlatform->m_lastPointerButtonPressSerial,
                                           static_cast<xdgToplevelResizeEdge>(edges));
}

eResizeEdge CWaylandWindow::edgeForPos(const Vector2D& local) const {
    if (!m_creationData.resizable)
        return HT_RESIZE_EDGE_NONE;

    const auto& sz = m_waylandState.logicalSize;
    if (sz.x <= 0 || sz.y <= 0)
        return HT_RESIZE_EDGE_NONE;

    uint32_t e = HT_RESIZE_EDGE_NONE;
    if (local.x < kResizeBorderPx)
        e |= HT_RESIZE_EDGE_LEFT;
    else if (local.x > sz.x - kResizeBorderPx)
        e |= HT_RESIZE_EDGE_RIGHT;
    if (local.y < kResizeBorderPx)
        e |= HT_RESIZE_EDGE_TOP;
    else if (local.y > sz.y - kResizeBorderPx)
        e |= HT_RESIZE_EDGE_BOTTOM;

    return static_cast<eResizeEdge>(e);
}

void CWaylandWindow::mouseMove(const Vector2D& local) {
    // run normal dispatch first so elements get a chance to claim the position.
    // hovered elements set their own cursor via updateFocus.
    IWaylandWindow::mouseMove(local);

    if (!m_creationData.resizable)
        return;

    // an element claimed the position. don't steal it for resize.
    if (m_mainHoverElement && m_mainHoverElement->m_el)
        return;

    const auto edge = edgeForPos(local);
    if (edge == HT_RESIZE_EDGE_NONE)
        return;

    ePointerShape shape = HT_POINTER_ARROW;
    switch (edge) {
        case HT_RESIZE_EDGE_TOP:
        case HT_RESIZE_EDGE_BOTTOM: shape = HT_POINTER_RESIZE_NS; break;
        case HT_RESIZE_EDGE_LEFT:
        case HT_RESIZE_EDGE_RIGHT: shape = HT_POINTER_RESIZE_EW; break;
        case HT_RESIZE_EDGE_TOP_LEFT:
        case HT_RESIZE_EDGE_BOTTOM_RIGHT: shape = HT_POINTER_RESIZE_NWSE; break;
        case HT_RESIZE_EDGE_TOP_RIGHT:
        case HT_RESIZE_EDGE_BOTTOM_LEFT: shape = HT_POINTER_RESIZE_NESW; break;
        default: break;
    }
    setCursor(shape);
}

void CWaylandWindow::mouseButton(const Input::eMouseButton button, bool state) {
    if (m_creationData.resizable && state && button == Input::MOUSE_BUTTON_LEFT) {
        // an element claimed the position. don't steal the click for resize.
        const bool elementClaimed = m_mainHoverElement && m_mainHoverElement->m_el;
        if (!elementClaimed) {
            const auto edge = edgeForPos(m_mousePos);
            if (edge != HT_RESIZE_EDGE_NONE) {
                startInteractiveResize(edge);
                return;
            }
        }
    }

    IWaylandWindow::mouseButton(button, state);
}

void CWaylandWindow::onPreRender() {
    // capture before the base call clears m_needsReposition
    const bool hadReposition = !m_needsReposition.empty();

    IWaylandWindow::onPreRender();

    if (!m_creationData.autosize || !hadReposition || m_pendingResize.pending)
        return;

    // skip when the compositor must control our size. MAXIMIZED is intentionally
    // not in this list: hyprland tags floating windows with it and we'd never
    // autosize otherwise. on strict compositors this risks a protocol error.
    if (m_isFullscreen || m_isResizing)
        return;

    if (!m_rootElement)
        return;

    // pass Vector2D{0, 0} so PERCENT-sized descendants contribute nothing to the
    // natural size; only ABSOLUTE and AUTO content shape the window.
    const auto preferred = m_rootElement->preferredSize(Vector2D{0, 0});

    if (!preferred || preferred->x <= 0 || preferred->y <= 0)
        return;

    if (*preferred == m_waylandState.logicalSize)
        return;

    setSize(*preferred);
}
