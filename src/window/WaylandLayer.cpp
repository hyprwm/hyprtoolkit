#include "WaylandLayer.hpp"
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

CWaylandLayer::CWaylandLayer(const SWindowCreationData& data) : m_creationData(data) {
    m_rootElement = CNullBuilder::begin()->commence();
}

CWaylandLayer::~CWaylandLayer() {
    close();

    if (g_waylandPlatform)
        std::erase_if(g_waylandPlatform->m_layers, [this](const auto& e) { return e.get() == this; });
}

void CWaylandLayer::open() {
    if (m_open)
        return;

    m_open = true;

    if (!m_creationData.preferredSize) {
        g_logger->log(HT_LOG_ERROR, "layer opening failed: no preferred size.");
        return;
    }

    m_rootElement->impl->window = m_self;
    m_rootElement->impl->breadthfirst([this](SP<IElement> e) { e->impl->window = m_self; });

    if (!m_waylandState.surface) {
        m_waylandState.surface = makeShared<CCWlSurface>(g_waylandPlatform->m_waylandState.compositor->sendCreateSurface());

        if (!m_waylandState.surface->resource()) {
            g_logger->log(HT_LOG_ERROR, "layer opening failed: no surface given. Errno: {}", errno);
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

            if (!SAMESCALE && m_layerState.configured)
                onScaleUpdate();
        });

        m_waylandState.viewport = makeShared<CCWpViewport>(g_waylandPlatform->m_waylandState.viewporter->sendGetViewport(m_waylandState.surface->resource()));
    } else
        m_waylandState.surface->sendAttach(nullptr, 0, 0);

    m_layerState.layerSurface = makeShared<CCZwlrLayerSurfaceV1>(g_waylandPlatform->m_waylandState.layerShell->sendGetLayerSurface(
        m_waylandState.surface->proxy(), nullptr, sc<zwlrLayerShellV1Layer>(m_creationData.layer), m_creationData.class_.c_str()));

    if (!m_layerState.layerSurface->resource()) {
        g_logger->log(HT_LOG_ERROR, "layer opening failed: no ls resource. Errno: {}", errno);
        return;
    }

    m_layerState.layerSurface->sendSetSize(m_creationData.preferredSize->x, m_creationData.preferredSize->y);
    m_layerState.layerSurface->sendSetAnchor(sc<zwlrLayerSurfaceV1Anchor>(m_creationData.anchor));
    m_layerState.layerSurface->sendSetExclusiveZone(m_creationData.exclusiveZone);
    m_layerState.layerSurface->sendSetMargin(m_creationData.marginTopLeft.y, m_creationData.marginBottomRight.x, m_creationData.marginBottomRight.y,
                                             m_creationData.marginTopLeft.x);
    m_layerState.layerSurface->sendSetKeyboardInteractivity(sc<zwlrLayerSurfaceV1KeyboardInteractivity>(m_creationData.kbInteractive));
    if (m_creationData.exclusiveEdge > 0)
        m_layerState.layerSurface->sendSetExclusiveEdge(sc<zwlrLayerSurfaceV1Anchor>(m_creationData.exclusiveEdge));

    m_waylandState.surface->sendCommit();

    m_layerState.layerSurface->setConfigure([this](CCZwlrLayerSurfaceV1* r, uint32_t serial, uint32_t w, uint32_t h) {
        g_logger->log(HT_LOG_DEBUG, "wayland: configure layer with {}x{}", w, h);
        m_layerState.configured = true;
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

        if (m_waylandState.logicalSize == Vector2D{sc<float>(w), sc<float>(h)})
            return;

        m_layerState.layerSurface->sendAckConfigure(serial);

        configure(Vector2D{sc<float>(w), sc<float>(h)}, m_waylandState.serial);

        m_events.resized.emit(m_waylandState.logicalSize);
    });

    m_layerState.layerSurface->setClosed([this](CCZwlrLayerSurfaceV1* r) {
        close();
        m_events.layerClosed.emit();
    });
}

void CWaylandLayer::close() {
    if (!m_open)
        return;

    m_open = false;

    m_waylandState.frameCallback.reset();

    m_layerState.layerSurface.reset();
    m_waylandState.logicalSize = {};
    m_layerState.configured    = false;
}

void CWaylandLayer::render() {
    if (!m_layerState.configured)
        return;

    IWaylandWindow::render();
}
