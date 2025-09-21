#include "WaylandWindow.hpp"

#include <hyprtoolkit/element/Rectangle.hpp>

#include "../element/Element.hpp"
#include "../core/platforms/WaylandPlatform.hpp"
#include "../core/InternalBackend.hpp"
#include "../renderer/Renderer.hpp"
#include "../core/AnimationManager.hpp"

#include "../Macros.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

CWaylandBuffer::CWaylandBuffer(SP<Aquamarine::IBuffer> buffer) : m_buffer(buffer) {
    auto params = makeShared<CCZwpLinuxBufferParamsV1>(g_waylandPlatform->m_waylandState.dmabuf->sendCreateParams());

    if (!params) {
        g_logger->log(HT_LOG_ERROR, "WaylandBuffer: failed to query params");
        return;
    }

    auto attrs = buffer->dmabuf();

    for (int i = 0; i < attrs.planes; ++i) {
        params->sendAdd(attrs.fds.at(i), i, attrs.offsets.at(i), attrs.strides.at(i), attrs.modifier >> 32, attrs.modifier & 0xFFFFFFFF);
    }

    m_waylandState.buffer = makeShared<CCWlBuffer>(params->sendCreateImmed(attrs.size.x, attrs.size.y, attrs.format, (zwpLinuxBufferParamsV1Flags)0));

    m_waylandState.buffer->setRelease([this](CCWlBuffer* r) { pendingRelease = false; });

    params->sendDestroy();
}

CWaylandBuffer::~CWaylandBuffer() {
    if (m_waylandState.buffer && m_waylandState.buffer->resource())
        m_waylandState.buffer->sendDestroy();
}

bool CWaylandBuffer::good() {
    return m_waylandState.buffer && m_waylandState.buffer->resource();
}

CWaylandWindow::CWaylandWindow(const SWindowCreationData& data) : m_creationData(data) {

    // m_waylandState.swapchain = Aquamarine::CSwapchain::create(g_waylandPlatform->m_allocator, g_backend->m_aqBackend->getImplementations().at(0));

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

    m_waylandState.xdgToplevel = makeShared<CCXdgToplevel>(m_waylandState.xdgSurface->sendGetToplevel());

    if (!m_waylandState.xdgToplevel->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no xdgToplevel given. Errno: {}", errno);
        return;
    }

    m_waylandState.xdgToplevel->setWmCapabilities([](CCXdgToplevel* r, wl_array* arr) { g_logger->log(HT_LOG_DEBUG, "window opening failed: wm_capabilities received"); });

    m_waylandState.xdgToplevel->setConfigure([this](CCXdgToplevel* r, int32_t w, int32_t h, wl_array* arr) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure toplevel with {}x{}", w, h);
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

        if (m_waylandState.logicalSize == Vector2D{w, h})
            return;

        configure({w, h}, m_waylandState.serial);
    });

    m_waylandState.fractional = makeShared<CCWpFractionalScaleV1>(g_waylandPlatform->m_waylandState.fractional->sendGetFractionalScale(m_waylandState.surface->resource()));

    m_waylandState.fractional->setPreferredScale([this](CCWpFractionalScaleV1*, uint32_t scale) {
        const bool SAMESCALE = m_fractionalScale == scale / 120.0;
        m_fractionalScale    = scale / 120.0;

        g_logger->log(HT_LOG_DEBUG, "window: got fractional scale: {:.1f}%", m_fractionalScale * 100.F);

        if (!SAMESCALE)
            onScaleUpdate();
    });

    m_waylandState.viewport = makeShared<CCWpViewport>(g_waylandPlatform->m_waylandState.viewporter->sendGetViewport(m_waylandState.surface->resource()));

    m_waylandState.xdgToplevel->setClose([this](CCXdgToplevel* r) { m_events.closed.emit(); });

    m_waylandState.xdgToplevel->sendSetTitle(m_creationData.title.c_str());
    m_waylandState.xdgToplevel->sendSetAppId(m_creationData.class_.c_str());

    if (data.minSize)
        m_waylandState.xdgToplevel->sendSetMinSize(data.minSize->x, data.minSize->y);
    if (data.maxSize)
        m_waylandState.xdgToplevel->sendSetMaxSize(data.maxSize->x, data.maxSize->y);

    auto inputRegion = makeShared<CCWlRegion>(g_waylandPlatform->m_waylandState.compositor->sendCreateRegion());
    inputRegion->sendAdd(0, 0, INT32_MAX, INT32_MAX);

    m_waylandState.surface->sendSetInputRegion(inputRegion.get());
    m_waylandState.surface->sendAttach(nullptr, 0, 0);
    m_waylandState.surface->sendCommit();

    inputRegion->sendDestroy();

    m_rootElement = CRectangleElement::create();
}

CWaylandWindow::~CWaylandWindow() {
    if (m_waylandState.eglSurface)
        eglDestroySurface(g_pEGL->eglDisplay, m_waylandState.eglSurface);

    if (m_waylandState.eglWindow)
        wl_egl_window_destroy(m_waylandState.eglWindow);

    m_waylandState.surface->sendAttach(nullptr, 0, 0);
    m_waylandState.surface->sendCommit();
    m_waylandState.frameCallback.reset();

    if (m_waylandState.xdgToplevel)
        m_waylandState.xdgToplevel->sendDestroy();
    if (m_waylandState.xdgSurface)
        m_waylandState.xdgSurface->sendDestroy();
    if (m_waylandState.surface)
        m_waylandState.surface->sendDestroy();
}

void CWaylandWindow::onScaleUpdate() {
    configure(m_waylandState.logicalSize, m_waylandState.serial);
}

void CWaylandWindow::configure(const Vector2D& size, uint32_t serial) {

    m_waylandState.logicalSize  = size;
    m_waylandState.appliedScale = m_fractionalScale;

    m_waylandState.size = (size * m_fractionalScale).floor();
    m_waylandState.viewport->sendSetDestination(m_waylandState.logicalSize.x, m_waylandState.logicalSize.y);
    m_waylandState.surface->sendSetBufferScale(1);

    resizeSwapchain(m_waylandState.size);
    damageEntire();

    m_rootElement->reposition({0, 0, m_waylandState.logicalSize.x, m_waylandState.logicalSize.y});

    if (!m_waylandState.eglWindow) {
        m_waylandState.eglWindow = wl_egl_window_create((wl_surface*)m_waylandState.surface->resource(), m_waylandState.size.x, m_waylandState.size.y);
        RASSERT(m_waylandState.eglWindow, "Couldn't create eglWindow");
    } else
        wl_egl_window_resize(m_waylandState.eglWindow, m_waylandState.size.x, m_waylandState.size.y, 0, 0);

    if (!m_waylandState.eglSurface) {
        m_waylandState.eglSurface = g_pEGL->eglCreatePlatformWindowSurfaceEXT(g_pEGL->eglDisplay, g_pEGL->eglConfig, m_waylandState.eglWindow, nullptr);
        RASSERT(m_waylandState.eglSurface, "Couldn't create eglSurface");
    }

    // m_waylandState.surface->sendAttach(m_waylandState.wlBuffers[0]->m_waylandState.buffer.get(), 0, 0);
    // m_waylandState.surface->sendCommit();

    render();
}

void CWaylandWindow::resizeSwapchain(const Vector2D& pixelSize) {
    m_damageRing.setSize(pixelSize);

    // m_waylandState.swapchain->reconfigure(Aquamarine::SSwapchainOptions{
    //     .length = 2,
    //     .size   = pixelSize,
    //     .format = g_waylandPlatform->m_dmabufFormats.at(0).drmFormat,
    // });

    // for (size_t i = 0; i < m_waylandState.wlBuffers.size(); ++i) {
    //     if (!m_waylandState.wlBuffers.at(i))
    //         m_waylandState.wlBuffers[i] = makeShared<CWaylandBuffer>(m_waylandState.swapchain->next(nullptr));
    // }
}

void CWaylandWindow::render() {
    if (m_waylandState.frameCallback)
        return;

    onPreRender();

    // FIXME: this is required because we use the stupid fucking eglSwapBuffers thing
    damageEntire();

    m_needsFrame = false;

    g_pEGL->makeCurrent(m_waylandState.eglSurface);

    g_renderer->beginRendering(m_self.lock());

    m_waylandState.frameCallback = makeShared<CCWlCallback>(m_waylandState.surface->sendFrame());
    m_waylandState.frameCallback->setDone([this](CCWlCallback* r, uint32_t frameTime) { onCallback(); });

    m_damageRing.getBufferDamage(DAMAGE_RING_PREVIOUS_LEN).forEachRect([this](const pixman_box32_t box) {
        m_waylandState.surface->sendDamageBuffer(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
    });

    g_renderer->endRendering();

    eglSwapBuffers(g_pEGL->eglDisplay, m_waylandState.eglSurface);

    // print frame time
    if (Env::isTrace()) {
        auto dur   = std::chrono::steady_clock::now() - m_lastFrame;
        auto durMs = std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / 1000.F;
        g_logger->log(HT_LOG_TRACE, "wayland: last frame took {:.2f}ms, FPS: {:.2f}", durMs, 1000.F / durMs);
        m_lastFrame = std::chrono::steady_clock::now();
    }

    m_needsFrame = g_animationManager->shouldTickForNext();
}

void CWaylandWindow::onCallback() {
    m_waylandState.frameCallback.reset();

    if (m_needsFrame)
        render();
}

Hyprutils::Math::Vector2D CWaylandWindow::pixelSize() {
    return m_waylandState.size;
}

float CWaylandWindow::scale() {
    return m_fractionalScale;
}
