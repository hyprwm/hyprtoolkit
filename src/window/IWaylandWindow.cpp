#include "IWaylandWindow.hpp"
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

void IWaylandWindow::onScaleUpdate() {
    configure(m_waylandState.logicalSize, m_waylandState.serial);
}

void IWaylandWindow::configure(const Vector2D& size, uint32_t serial) {

    m_waylandState.logicalSize  = size;
    m_waylandState.appliedScale = m_fractionalScale;

    m_waylandState.size = (size * m_fractionalScale).floor();
    m_waylandState.viewport->sendSetDestination(m_waylandState.logicalSize.x, m_waylandState.logicalSize.y);
    m_waylandState.surface->sendSetBufferScale(1);

    resizeSwapchain(m_waylandState.size);
    damageEntire();

    m_rootElement->reposition({0, 0, m_waylandState.logicalSize.x, m_waylandState.logicalSize.y});
}

void IWaylandWindow::resizeSwapchain(const Vector2D& pixelSize) {
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

void IWaylandWindow::render() {
    if (m_waylandState.frameCallback)
        return;

    auto currentBuffer    = m_waylandState.wlBuffers[m_waylandState.bufIdx];
    m_waylandState.bufIdx = (m_waylandState.bufIdx + 1) % 2;

    onPreRender();

    if (!currentBuffer)
        return;

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
}

void IWaylandWindow::onCallback() {
    m_waylandState.frameCallback.reset();

    if (m_needsFrame)
        render();
}

Hyprutils::Math::Vector2D IWaylandWindow::pixelSize() {
    return m_waylandState.size;
}

float IWaylandWindow::scale() {
    return m_fractionalScale;
}

void IWaylandWindow::setCursor(ePointerShape shape) {
    g_waylandPlatform->setCursor(shape);
}

SP<IWindow> IWaylandWindow::openPopup(const SWindowCreationData& data) {
    auto x    = makeShared<CWaylandPopup>(data, reinterpretPointerCast<CWaylandWindow>(m_self.lock()));
    x->m_self = x;
    m_popups.emplace_back(x);
    return x;
}

void IWaylandWindow::mouseButton(const Input::eMouseButton button, bool state) {
    if (m_popups.empty() || !state || m_ignoreNextButtonEvent) {
        m_ignoreNextButtonEvent = false;
        IToolkitWindow::mouseButton(button, state);
        return;
    }

    for (const auto& p : m_popups) {
        if (p)
            p->close();
    }

    m_popups.clear();

    m_ignoreNextButtonEvent = true;
}

void IWaylandWindow::mouseMove(const Hyprutils::Math::Vector2D& local) {
    if (!m_popups.empty())
        return;

    IToolkitWindow::mouseMove(local);
}

void IWaylandWindow::mouseAxis(const Input::eAxisAxis axis, float delta) {
    if (!m_popups.empty())
        return;

    IToolkitWindow::mouseAxis(axis, delta);
}

void IWaylandWindow::setIMTo(const Hyprutils::Math::CBox& box, const std::string& str, size_t cursor) {
    if (!g_waylandPlatform->m_waylandState.imState.enabled) {
        g_waylandPlatform->m_waylandState.textInput->sendEnable();
        g_waylandPlatform->m_waylandState.imState.enabled = true;
    }
    g_waylandPlatform->m_waylandState.textInput->sendSetCursorRectangle(box.x, box.y, box.w, box.h);
    g_waylandPlatform->m_waylandState.textInput->sendCommit();

    m_currentInput       = str;
    m_currentInputCursor = cursor;
}

void IWaylandWindow::resetIM() {
    if (g_waylandPlatform->m_waylandState.imState.enabled) {
        g_waylandPlatform->m_waylandState.textInput->sendDisable();
        g_waylandPlatform->m_waylandState.imState.enabled = false;
    }

    m_currentInput       = "";
    m_currentInputCursor = 0;
}
