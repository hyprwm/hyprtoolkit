#include "WaylandPlatform.hpp"

#include <hyprutils/memory/Casts.hpp>

#include "../InternalBackend.hpp"
#include "../Input.hpp"
#include "../../window/WaylandWindow.hpp"
#include "../../Macros.hpp"

#include <xf86drm.h>
#include <cstring>
#include <sys/mman.h>
#include <fcntl.h>

using namespace Hyprtoolkit;
using namespace Hyprutils::Math;

static std::string fourccToName(uint32_t drmFormat) {
    auto        fmt  = drmGetFormatName(drmFormat);
    std::string name = fmt ? fmt : "unknown";
    free(fmt);
    return name;
}

bool CWaylandPlatform::attempt() {
    g_logger->log(HT_LOG_DEBUG, "Starting the Wayland platform");

    m_waylandState.display = wl_display_connect(nullptr);

    if (!m_waylandState.display) {
        g_logger->log(HT_LOG_ERROR, "Wayland platform cannot start: wl_display_connect failed (is a wayland compositor running?)");
        return false;
    }

    auto XDGCURRENTDESKTOP = getenv("XDG_CURRENT_DESKTOP");
    g_logger->log(HT_LOG_DEBUG, "Connected to a wayland compositor: {}", (XDGCURRENTDESKTOP ? XDGCURRENTDESKTOP : "unknown (XDG_CURRENT_DEKSTOP unset?)"));

    m_waylandState.registry = makeShared<CCWlRegistry>((wl_proxy*)wl_display_get_registry(m_waylandState.display));

    g_logger->log(HT_LOG_DEBUG, "Got registry at 0x{:x}", (uintptr_t)m_waylandState.registry->resource());

    m_waylandState.registry->setGlobal([this](CCWlRegistry* r, uint32_t id, const char* name, uint32_t version) {
        TRACE(g_logger->log(HT_LOG_TRACE, " | received global: {} (version {}) with id {}", name, version, id));

        const std::string NAME = name;

        if (NAME == "wl_seat") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 9, id));
            m_waylandState.seat = makeShared<CCWlSeat>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wl_seat_interface, 9));
            initSeat();
        } else if (NAME == "xdg_wm_base") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 6, id));
            m_waylandState.xdg = makeShared<CCXdgWmBase>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &xdg_wm_base_interface, 6));
            initShell();
        } else if (NAME == "wl_compositor") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 6, id));
            m_waylandState.compositor = makeShared<CCWlCompositor>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wl_compositor_interface, 6));
        } else if (NAME == "wl_shm") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.shm = makeShared<CCWlShm>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wl_shm_interface, 1));
        } else if (NAME == "wp_viewporter") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.viewporter = makeShared<CCWpViewporter>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wp_viewporter_interface, 1));
        } else if (NAME == wp_fractional_scale_manager_v1_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.fractional = makeShared<CCWpFractionalScaleManagerV1>(
                (wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wp_fractional_scale_manager_v1_interface, 1));
        } else if (NAME == wp_cursor_shape_manager_v1_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.cursorShapeMgr =
                makeShared<CCWpCursorShapeManagerV1>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wp_cursor_shape_manager_v1_interface, 1));
        } else if (NAME == "zwp_linux_dmabuf_v1") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 4, id));
            m_waylandState.dmabuf =
                makeShared<CCZwpLinuxDmabufV1>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &zwp_linux_dmabuf_v1_interface, 4));
            if (!initDmabuf()) {
                g_logger->log(HT_LOG_ERROR, "Wayland platform cannot start: zwp_linux_dmabuf_v1 init failed");
                m_waylandState.dmabufFailed = true;
            }
        }
    });
    m_waylandState.registry->setGlobalRemove([](CCWlRegistry* r, uint32_t id) { g_logger->log(HT_LOG_DEBUG, "Global {} removed", id); });

    wl_display_roundtrip(m_waylandState.display);

    if (!m_waylandState.xdg || !m_waylandState.compositor || !m_waylandState.seat || !m_waylandState.dmabuf || m_waylandState.dmabufFailed || !m_waylandState.shm) {
        g_logger->log(HT_LOG_ERROR, "Wayland platform cannot start: Missing protocols");
        return false;
    }

    dispatchEvents();

    return true;
}

CWaylandPlatform::~CWaylandPlatform() {
    const auto DPY = m_waylandState.display;
    m_waylandState = {};
    wl_display_disconnect(DPY);

    if (m_drmState.fd >= 0)
        close(m_drmState.fd);
}

bool CWaylandPlatform::dispatchEvents() {
    wl_display_flush(m_waylandState.display);

    if (wl_display_prepare_read(m_waylandState.display) == 0) {
        wl_display_read_events(m_waylandState.display);
        wl_display_dispatch_pending(m_waylandState.display);
    } else
        wl_display_dispatch(m_waylandState.display);

    int ret = 0;
    do {
        ret = wl_display_dispatch_pending(m_waylandState.display);
        wl_display_flush(m_waylandState.display);
    } while (ret > 0);

    // dispatch frames
    for (auto const& f : m_idleCallbacks) {
        f();
    }
    m_idleCallbacks.clear();

    return true;
}

SP<CWaylandWindow> CWaylandPlatform::windowForSurf(wl_proxy* proxy) {
    for (const auto& w : m_windows) {
        if (w->m_waylandState.surface && w->m_waylandState.surface->resource() == proxy)
            return w.lock();

        for (const auto& p : w->m_popups) {
            if (!p)
                continue;
            auto pp = reinterpretPointerCast<CWaylandWindow>(p.lock());
            if (pp->m_waylandState.surface && pp->m_waylandState.surface->resource() == proxy)
                return pp;
        }
    }
    return nullptr;
}

void CWaylandPlatform::initSeat() {
    m_waylandState.seat->setCapabilities([this](CCWlSeat* r, wl_seat_capability cap) {
        const bool HAS_KEYBOARD = ((uint32_t)cap) & WL_SEAT_CAPABILITY_KEYBOARD;
        const bool HAS_POINTER  = ((uint32_t)cap) & WL_SEAT_CAPABILITY_POINTER;

        if (HAS_KEYBOARD && !m_waylandState.keyboard) {
            m_waylandState.keyboard = makeShared<CCWlKeyboard>(m_waylandState.seat->sendGetKeyboard());

            // TODO:

        } else if (!HAS_KEYBOARD && m_waylandState.keyboard)
            m_waylandState.keyboard.reset();

        if (HAS_POINTER && !m_waylandState.pointer) {
            m_waylandState.pointer = makeShared<CCWlPointer>(m_waylandState.seat->sendGetPointer());

            m_waylandState.pointer->setEnter([this](CCWlPointer* r, uint32_t serial, wl_proxy* surf, wl_fixed_t x, wl_fixed_t y) {
                auto w = windowForSurf(surf);

                if (!w)
                    return;

                Vector2D local = {wl_fixed_to_double(x), wl_fixed_to_double(y)};

                w->mouseEnter(local);
                m_currentWindow   = w;
                m_lastEnterSerial = serial;

                setCursor(HT_POINTER_ARROW);
            });

            m_waylandState.pointer->setLeave([this](CCWlPointer* r, uint32_t serial, wl_proxy* surf) {
                auto w = windowForSurf(surf);

                if (!w)
                    return;

                w->mouseLeave();
                m_currentWindow.reset();
            });

            m_waylandState.pointer->setMotion([this](CCWlPointer* r, uint32_t time, wl_fixed_t x, wl_fixed_t y) {
                if (!m_currentWindow)
                    return;

                Vector2D local = {wl_fixed_to_double(x), wl_fixed_to_double(y)};

                m_currentWindow->mouseMove(local);
            });

            m_waylandState.pointer->setButton([this](CCWlPointer* r, uint32_t serial, uint32_t time, uint32_t button, wl_pointer_button_state state) {
                if (!m_currentWindow)
                    return;

                m_currentWindow->mouseButton(Input::buttonFromWayland(button), state == WL_POINTER_BUTTON_STATE_PRESSED);
            });

            m_waylandState.pointer->setAxis([this](CCWlPointer* r, uint32_t serial, wl_pointer_axis axis, wl_fixed_t delta) {
                if (!m_currentWindow)
                    return;

                m_currentWindow->mouseAxis(axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL ? Input::AXIS_AXIS_HORIZONTAL : Input::AXIS_AXIS_VERTICAL, wl_fixed_to_double(delta));
            });

            m_waylandState.cursorShapeDev = makeShared<CCWpCursorShapeDeviceV1>(m_waylandState.cursorShapeMgr->sendGetPointer(m_waylandState.pointer->resource()));

        } else if (!HAS_POINTER && m_waylandState.pointer) {
            m_waylandState.pointer.reset();
            m_waylandState.cursorShapeDev.reset();
        }
    });
}

void CWaylandPlatform::initShell() {
    m_waylandState.xdg->setPing([](CCXdgWmBase* r, uint32_t serial) { r->sendPong(serial); });
}

bool CWaylandPlatform::initDmabuf() {
    m_waylandState.dmabufFeedback = makeShared<CCZwpLinuxDmabufFeedbackV1>(m_waylandState.dmabuf->sendGetDefaultFeedback());
    if (!m_waylandState.dmabufFeedback) {
        g_logger->log(HT_LOG_ERROR, "initDmabuf: failed to get default feedback");
        return false;
    }

    m_waylandState.dmabufFeedback->setDone([this](CCZwpLinuxDmabufFeedbackV1* r) {
        // no-op
        g_logger->log(HT_LOG_DEBUG, "zwp_linux_dmabuf_v1: Got done");
    });

    m_waylandState.dmabufFeedback->setMainDevice([this](CCZwpLinuxDmabufFeedbackV1* r, wl_array* deviceArr) {
        g_logger->log(HT_LOG_DEBUG, "zwp_linux_dmabuf_v1: Got main device");

        dev_t device;
        ASSERT(deviceArr->size == sizeof(device));
        memcpy(&device, deviceArr->data, sizeof(device));

        drmDevice* drmDev;
        if (drmGetDeviceFromDevId(device, /* flags */ 0, &drmDev) != 0) {
            g_logger->log(HT_LOG_ERROR, "zwp_linux_dmabuf_v1: drmGetDeviceFromDevId failed");
            return;
        }

        const char* name = nullptr;
        if (drmDev->available_nodes & (1 << DRM_NODE_RENDER))
            name = drmDev->nodes[DRM_NODE_RENDER];
        else {
            // Likely a split display/render setup. Pick the primary node and hope
            // Mesa will open the right render node under-the-hood.
            ASSERT(drmDev->available_nodes & (1 << DRM_NODE_PRIMARY));
            name = drmDev->nodes[DRM_NODE_PRIMARY];
            g_logger->log(HT_LOG_WARNING, "zwp_linux_dmabuf_v1: DRM device has no render node, using primary.");
        }

        if (!name) {
            g_logger->log(HT_LOG_ERROR, "zwp_linux_dmabuf_v1: no node name");
            return;
        }

        m_drmState.nodeName = name;

        drmFreeDevice(&drmDev);

        g_logger->log(HT_LOG_DEBUG, "zwp_linux_dmabuf_v1: Got node {}", m_drmState.nodeName);
    });

    m_waylandState.dmabufFeedback->setFormatTable([this](CCZwpLinuxDmabufFeedbackV1* r, int32_t fd, uint32_t size) {
#pragma pack(push, 1)
        struct wlDrmFormatMarshalled {
            uint32_t drmFormat;
            char     pad[4];
            uint64_t modifier;
        };
#pragma pack(pop)
        static_assert(sizeof(wlDrmFormatMarshalled) == 16);

        auto formatTable = mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (formatTable == MAP_FAILED) {
            g_logger->log(HT_LOG_ERROR, "zwp_linux_dmabuf_v1: Failed to mmap the format table");
            return;
        }

        const auto FORMATS = (wlDrmFormatMarshalled*)formatTable;

        for (size_t i = 0; i < size / 16; ++i) {
            auto& fmt = FORMATS[i];

            auto  modName = drmGetFormatModifierName(fmt.modifier);
            g_logger->log(HT_LOG_DEBUG, "zwp_linux_dmabuf_v1: Got format {} with modifier {}", fourccToName(fmt.drmFormat), modName ? modName : "UNKNOWN");
            free(modName);

            auto it = std::ranges::find_if(m_dmabufFormats, [&fmt](const auto& e) { return e.drmFormat == fmt.drmFormat; });
            if (it == m_dmabufFormats.end()) {
                m_dmabufFormats.emplace_back(Aquamarine::SDRMFormat{.drmFormat = fmt.drmFormat, .modifiers = {fmt.modifier}});
                continue;
            }

            it->modifiers.emplace_back(fmt.modifier);
        }

        munmap(formatTable, size);
    });

    wl_display_roundtrip(m_waylandState.display);

    if (!m_drmState.nodeName.empty()) {
        m_drmState.fd = open(m_drmState.nodeName.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
        if (m_drmState.fd < 0) {
            g_logger->log(HT_LOG_ERROR, "zwp_linux_dmabuf_v1: Failed to open node {}", m_drmState.nodeName);
            return false;
        }

        g_logger->log(HT_LOG_DEBUG, "zwp_linux_dmabuf_v1: opened node {} with fd {}", m_drmState.nodeName, m_drmState.fd);
    }

    m_allocator = Aquamarine::CGBMAllocator::create(m_drmState.fd, g_backend->m_aqBackend);

    auto nullBackend = reinterpretPointerCast<Aquamarine::CNullBackend>(g_backend->m_aqBackend->getImplementations().at(0));

    nullBackend->setFormats(m_dmabufFormats);

    return true;
}

void CWaylandPlatform::setCursor(ePointerShape shape) {
    if (!m_waylandState.cursorShapeDev)
        return;

    wpCursorShapeDeviceV1Shape wlShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_DEFAULT;

    switch (shape) {
        case HT_POINTER_ARROW: break;
        case HT_POINTER_POINTER: wlShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_POINTER; break;
        default: break;
    }

    m_waylandState.cursorShapeDev->sendSetShape(m_lastEnterSerial, wlShape);
}
