#include "WaylandPopup.hpp"

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

    m_open                 = true;
    m_needsFirstReposition = true;

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

    m_waylandState.xdgPositioner = makeShared<CCXdgPositioner>(g_waylandPlatform->m_waylandState.xdg->sendCreatePositioner());

    m_waylandState.xdgPositioner->sendSetAnchorRect(m_creationData.pos.x, m_creationData.pos.y, 1, 1);
    m_waylandState.xdgPositioner->sendSetAnchor(XDG_POSITIONER_ANCHOR_TOP_LEFT);
    m_waylandState.xdgPositioner->sendSetGravity(XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT);
    m_waylandState.xdgPositioner->sendSetSize(m_creationData.size.x, m_creationData.size.y);
    m_waylandState.xdgPositioner->sendSetConstraintAdjustment(
        (xdgPositionerConstraintAdjustment)(XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X));

    m_waylandState.xdgPopup = makeShared<CCXdgPopup>(m_waylandState.xdgSurface->sendGetPopup(m_parent->m_waylandState.xdgSurface.get(), m_waylandState.xdgPositioner.get()));

    if (!m_waylandState.xdgPopup->resource()) {
        g_logger->log(HT_LOG_ERROR, "window opening failed: no xdgToplevel given. Errno: {}", errno);
        return;
    }

    m_waylandState.xdgPopup->setConfigure([this](CCXdgPopup* r, int32_t x, int32_t y, int32_t w, int32_t h) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure toplevel with {}x{}", w, h);

        if (!m_waylandState.grabbed) {
            m_waylandState.grabbed = true;
            m_waylandState.xdgPopup->sendGrab(g_waylandPlatform->m_waylandState.seat->resource(), g_waylandPlatform->m_lastEnterSerial);
        }

        if (m_waylandState.logicalSize == Vector2D{w, h})
            return;

        configure({w, h}, m_waylandState.serial);

        m_events.resized.emit(m_waylandState.logicalSize);
    });

    m_waylandState.xdgPopup->setPopupDone([this](CCXdgPopup* r) { close(); });

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
    if (!m_open)
        return;

    m_open = false;

    m_waylandState.frameCallback.reset();

    m_events.popupClosed.emit();

    if (m_waylandState.xdgPopup)
        m_waylandState.xdgPopup->sendDestroy();
    if (m_waylandState.xdgSurface)
        m_waylandState.xdgSurface->sendDestroy();
    if (m_waylandState.surface)
        m_waylandState.surface->sendDestroy();

    m_waylandState = {};
}

void CWaylandPopup::onScaleUpdate() {
    configure(m_waylandState.logicalSize, m_waylandState.serial);
}

void CWaylandPopup::configure(const Vector2D& size, uint32_t serial) {

    m_waylandState.logicalSize  = size;
    m_waylandState.appliedScale = m_fractionalScale;

    m_waylandState.size = (size * m_fractionalScale).floor();
    m_waylandState.viewport->sendSetDestination(m_waylandState.logicalSize.x, m_waylandState.logicalSize.y);
    m_waylandState.surface->sendSetBufferScale(1);

    resizeSwapchain(m_waylandState.size);
    damageEntire();

    m_rootElement->reposition({0, 0, m_waylandState.logicalSize.x, m_waylandState.logicalSize.y});

    render();
}

void CWaylandPopup::resizeSwapchain(const Vector2D& pixelSize) {
    m_damageRing.setSize(pixelSize);

    if (!m_waylandState.swapchain)
        m_waylandState.swapchain = Aquamarine::CSwapchain::create(g_waylandPlatform->m_allocator, g_backend->m_aqBackend->getImplementations().at(0));

    m_waylandState.swapchain->reconfigure(Aquamarine::SSwapchainOptions{
        .length = 2,
        .size   = pixelSize,
        .format = g_waylandPlatform->m_dmabufFormats.at(0).drmFormat,
    });

    for (size_t i = 0; i < m_waylandState.wlBuffers.size(); ++i) {
        m_waylandState.wlBuffers[i] = makeShared<CWaylandBuffer>(m_waylandState.swapchain->next(nullptr));
    }
}

void CWaylandPopup::render() {
    if (m_waylandState.frameCallback)
        return;

    auto currentBuffer    = m_waylandState.wlBuffers[m_waylandState.bufIdx];
    m_waylandState.bufIdx = (m_waylandState.bufIdx + 1) % 2;

    onPreRender();

    m_needsFrame = false;

    g_renderer->beginRendering(m_self.lock(), currentBuffer->m_buffer.lock());

    m_waylandState.frameCallback = makeShared<CCWlCallback>(m_waylandState.surface->sendFrame());
    m_waylandState.frameCallback->setDone([this](CCWlCallback* r, uint32_t frameTime) { onCallback(); });

    m_damageRing.getBufferDamage(DAMAGE_RING_PREVIOUS_LEN).forEachRect([this](const pixman_box32_t box) {
        m_waylandState.surface->sendDamageBuffer(box.x1, box.y1, box.x2 - box.x1, box.y2 - box.y1);
    });

    g_renderer->endRendering();

    m_waylandState.surface->sendAttach(currentBuffer->m_waylandState.buffer.get(), 0, 0);
    m_waylandState.surface->sendCommit();

    //

    // print frame time
    if (Env::isTrace()) {
        auto dur   = std::chrono::steady_clock::now() - m_lastFrame;
        auto durMs = std::chrono::duration_cast<std::chrono::microseconds>(dur).count() / 1000.F;
        g_logger->log(HT_LOG_TRACE, "wayland: last frame took {:.2f}ms, FPS: {:.2f}", durMs, 1000.F / durMs);
        m_lastFrame = std::chrono::steady_clock::now();
    }

    m_needsFrame = m_needsFrame || g_animationManager->shouldTickForNext();

    if (m_needsFirstReposition)
        m_rootElement->reposition({{}, m_waylandState.logicalSize});
}

void CWaylandPopup::onCallback() {
    m_waylandState.frameCallback.reset();

    if (m_needsFrame)
        render();
}

Hyprutils::Math::Vector2D CWaylandPopup::pixelSize() {
    return m_waylandState.size;
}

float CWaylandPopup::scale() {
    return m_fractionalScale;
}

void CWaylandPopup::setCursor(ePointerShape shape) {
    g_waylandPlatform->setCursor(shape);
}

SP<IWindow> CWaylandPopup::openPopup(const SPopupCreationData& data) {
    return nullptr; //FIXME:
}
