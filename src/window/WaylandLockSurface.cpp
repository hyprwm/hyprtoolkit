#include "WaylandLockSurface.hpp"

#include <aquamarine/allocator/Swapchain.hpp>
#include <hyprtoolkit/element/Null.hpp>
#include <aquamarine/output/Output.hpp>

#include "../core/AnimationManager.hpp"
#include "../core/InternalBackend.hpp"
#include "../core/platforms/WaylandPlatform.hpp"
#include "../element/Element.hpp"
#include "../output/WaylandOutput.hpp"
#include "../renderer/Renderer.hpp"

#include "../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CWaylandLockSurface::CWaylandLockSurface(const SWindowCreationData& data) : m_outputId(data.prefferedOutputId) {
    m_rootElement = CNullBuilder::begin()->commence();
}

CWaylandLockSurface::~CWaylandLockSurface() {
    close();
}

void CWaylandLockSurface::open() {
    if (m_open)
        return;

    if (m_outputId == 0) {
        g_logger->log(HT_LOG_ERROR, "session lock missing prefferedOutputId.");
        return;
    }

    if (!g_waylandPlatform) {
        g_logger->log(HT_LOG_ERROR, "wayland platform not initialized.");
        return;
    }

    auto lockObject = g_waylandPlatform->aquireSessionLock();
    if (!lockObject)
        return;

    auto wlOutput = g_waylandPlatform->outputForId(m_outputId);
    if (!wlOutput.has_value() || !wlOutput.value())
        return;

    m_open = true;

    m_rootElement->impl->window = m_self;
    m_rootElement->impl->breadthfirst([this](SP<IElement> e) { e->impl->window = m_self; });

    if (!m_waylandState.surface) {
        m_waylandState.surface = makeShared<CCWlSurface>(g_waylandPlatform->m_waylandState.compositor->sendCreateSurface());
        if (!m_waylandState.surface->resource()) {
            g_logger->log(HT_LOG_ERROR, "lock surface opening failed: no surface given. Errno: {}", errno);
            return;
        }

        auto inputRegion = makeShared<CCWlRegion>(g_waylandPlatform->m_waylandState.compositor->sendCreateRegion());
        inputRegion->sendAdd(0, 0, INT32_MAX, INT32_MAX);

        m_waylandState.surface->sendSetInputRegion(inputRegion.get());

        m_waylandState.fractional = makeShared<CCWpFractionalScaleV1>(g_waylandPlatform->m_waylandState.fractional->sendGetFractionalScale(m_waylandState.surface->resource()));

        m_waylandState.fractional->setPreferredScale([this](CCWpFractionalScaleV1*, uint32_t scale) {
            const bool SAMESCALE = m_fractionalScale == scale / 120.0;
            m_fractionalScale    = scale / 120.0;

            g_logger->log(HT_LOG_DEBUG, "layer: got fractional scale: {:.1f}%", m_fractionalScale * 100.F);

            if (!SAMESCALE && m_lockSurfaceState.configured)
                onScaleUpdate();
        });

        m_waylandState.viewport = makeShared<CCWpViewport>(g_waylandPlatform->m_waylandState.viewporter->sendGetViewport(m_waylandState.surface->resource()));
    } else
        m_waylandState.surface->sendAttach(nullptr, 0, 0);

    m_lockSurfaceState.lockSurface =
        makeShared<CCExtSessionLockSurfaceV1>(lockObject->sendGetLockSurface(m_waylandState.surface->resource(), wlOutput.value()->m_wlOutput.resource()));
    if (!m_lockSurfaceState.lockSurface->resource()) {
        g_logger->log(HT_LOG_ERROR, "lock surface opening failed: no lock surface. Errno: {}", errno);
        return;
    }

    m_lockSurfaceState.lockSurface->setConfigure([this](CCExtSessionLockSurfaceV1* r, uint32_t serial, uint32_t w, uint32_t h) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure layer with {}x{}", w, h);
        m_lockSurfaceState.configured = true;
        if (w == 0 || h == 0) {
            g_logger->log(HT_LOG_ERROR, "configure: w/h is 0, for a lock surface is bogus. Still, trying with 1920x1080...");
            w = 1920;
            h = 1080;
        }

        if (m_waylandState.logicalSize == Vector2D{sc<float>(w), sc<float>(h)})
            return;

        m_lockSurfaceState.lockSurface->sendAckConfigure(serial);

        configure(Vector2D{sc<float>(w), sc<float>(h)}, m_waylandState.serial);

        m_events.resized.emit(m_waylandState.logicalSize);
    });
}

void CWaylandLockSurface::close() {
    if (!m_open)
        return;

    m_open = false;

    m_waylandState.frameCallback.reset();

    m_lockSurfaceState.lockSurface.reset();
    m_waylandState.logicalSize    = {};
    m_lockSurfaceState.configured = false;
}

void CWaylandLockSurface::render() {
    if (!m_lockSurfaceState.configured)
        return;

    IWaylandWindow::render();
}
