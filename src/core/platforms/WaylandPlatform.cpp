#include "WaylandPlatform.hpp"

#include <algorithm>
#include <expected>
#include <hyprutils/memory/Casts.hpp>
#include <optional>
#include <xkbcommon/xkbcommon-keysyms.h>

#include "../InternalBackend.hpp"
#include "../Input.hpp"
#include "../../element/Element.hpp"
#include "../../window/WaylandWindow.hpp"
#include "../../window/WaylandLayer.hpp"
#include "../../window/WaylandLockSurface.hpp"
#include "../../output/WaylandOutput.hpp"
#include "../../sessionLock/WaylandSessionLock.hpp"
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

    m_waylandState.seatState.xkbContext = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

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
        } else if (NAME == zwp_text_input_manager_v3_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.textInputManager =
                makeShared<CCZwpTextInputManagerV3>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &zwp_text_input_manager_v3_interface, 1));
        } else if (NAME == zwlr_layer_shell_v1_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 5, id));
            m_waylandState.layerShell =
                makeShared<CCZwlrLayerShellV1>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &zwlr_layer_shell_v1_interface, 5));
        } else if (NAME == wp_linux_drm_syncobj_manager_v1_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.syncobj = makeShared<CCWpLinuxDrmSyncobjManagerV1>(
                (wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wp_linux_drm_syncobj_manager_v1_interface, 1));
        } else if (NAME == "zwp_linux_dmabuf_v1") {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 4, id));
            m_waylandState.dmabuf =
                makeShared<CCZwpLinuxDmabufV1>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &zwp_linux_dmabuf_v1_interface, 4));
            if (!initDmabuf()) {
                g_logger->log(HT_LOG_ERROR, "Wayland platform cannot start: zwp_linux_dmabuf_v1 init failed");
                m_waylandState.dmabufFailed = true;
            }
        } else if (NAME == wl_output_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 4, id));
            auto newOutput = makeShared<CWaylandOutput>((wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &wl_output_interface, 4), id);
            m_outputs.emplace_back(newOutput);
            if (m_waylandState.initialized)
                g_backend->m_events.outputAdded.emit(newOutput);
        } else if (NAME == ext_session_lock_manager_v1_interface.name) {
            TRACE(g_logger->log(HT_LOG_TRACE, "  > binding to global: {} (version {}) with id {}", name, 1, id));
            m_waylandState.sessionLock = makeShared<CCExtSessionLockManagerV1>(
                (wl_proxy*)wl_registry_bind((wl_registry*)m_waylandState.registry->resource(), id, &ext_session_lock_manager_v1_interface, 1));
        }
    });
    m_waylandState.registry->setGlobalRemove([this](CCWlRegistry* r, uint32_t id) {
        TRACE(g_logger->log(HT_LOG_TRACE, "Global {} removed", id));

        if (m_sessionLockState)
            m_sessionLockState->onOutputRemoved(id);

        auto outputIt = std::ranges::find_if(m_outputs, [id](const auto& other) { return other->m_id == id; });
        if (outputIt != m_outputs.end()) {
            (*outputIt)->m_events.removed.emit();
            m_outputs.erase(outputIt);
        }
    });

    wl_display_roundtrip(m_waylandState.display);

    if (!m_waylandState.xdg || !m_waylandState.compositor || !m_waylandState.seat || !m_waylandState.dmabuf || m_waylandState.dmabufFailed || !m_waylandState.shm) {
        g_logger->log(HT_LOG_ERROR, "Wayland platform cannot start: Missing protocols");
        return false;
    }

    if (m_waylandState.textInputManager)
        initIM();

    dispatchEvents();

    m_waylandState.initialized = true;
    for (const auto& o : m_outputs) {
        g_backend->m_events.outputAdded.emit(o);
    }

    return true;
}

CWaylandPlatform::~CWaylandPlatform() {
    m_outputs.clear();

    if (m_drmState.fd >= 0)
        close(m_drmState.fd);

    if (m_waylandState.seatState.xkbState)
        xkb_state_unref(m_waylandState.seatState.xkbState);
    if (m_waylandState.seatState.xkbKeymap)
        xkb_keymap_unref(m_waylandState.seatState.xkbKeymap);
    if (m_waylandState.seatState.xkbContext)
        xkb_context_unref(m_waylandState.seatState.xkbContext);

    const auto DPY = m_waylandState.display;
    m_waylandState = {};
    wl_display_disconnect(DPY);
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

    return true;
}

SP<IWaylandWindow> CWaylandPlatform::windowForSurf(wl_proxy* proxy) {
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
    for (const auto& w : m_layers) {
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

    if (m_sessionLockState) {
        for (const auto& w : m_sessionLockState->m_lockSurfaces) {
            if (w->m_waylandState.surface && w->m_waylandState.surface->resource() == proxy)
                return w.lock();
        }
    }
    return nullptr;
}

std::optional<WP<CWaylandOutput>> CWaylandPlatform::outputForHandle(uint32_t handle) {
    for (const auto& o : m_outputs) {
        if (o->m_id == handle) {
            return o;
        }
    }
    return std::nullopt;
}

void CWaylandPlatform::initIM() {
    m_waylandState.textInput = makeShared<CCZwpTextInputV3>(m_waylandState.textInputManager->sendGetTextInput(m_waylandState.seat->resource()));

    m_waylandState.textInput->setPreeditString([this](CCZwpTextInputV3* r, const char* s, int32_t begin, int32_t end) {
        m_waylandState.imState.preeditBegin  = begin;
        m_waylandState.imState.preeditEnd    = end;
        m_waylandState.imState.preeditString = s;
    });

    m_waylandState.textInput->setDeleteSurroundingText([this](CCZwpTextInputV3* r, uint32_t bef, uint32_t aft) {
        m_waylandState.imState.deleteAfter  = aft;
        m_waylandState.imState.deleteBefore = bef;
    });

    m_waylandState.textInput->setCommitString([this](CCZwpTextInputV3* r, const char* s) { m_waylandState.imState.commitString = s; });

    m_waylandState.textInput->setDone([this](CCZwpTextInputV3* r, uint32_t serial) {
        if (!m_currentWindow || !m_currentWindow->m_keyboardFocus)
            return;

        // FIXME: this is incomplete. Very incomplete, but works for CJK IMEs just fine.

        const auto NEW_STR = m_waylandState.imState.commitString;

        if (NEW_STR.empty()) {
            m_currentWindow->m_keyboardFocus->imCommitNewText(m_waylandState.imState.preeditString);
            return;
        }

        m_waylandState.imState = {};

        m_currentWindow->m_keyboardFocus->imCommitNewText(NEW_STR);
        m_currentWindow->m_keyboardFocus->imApplyText();
    });
}

void CWaylandPlatform::initSeat() {
    m_waylandState.seat->setCapabilities([this](CCWlSeat* r, wl_seat_capability cap) {
        const bool HAS_KEYBOARD = ((uint32_t)cap) & WL_SEAT_CAPABILITY_KEYBOARD;
        const bool HAS_POINTER  = ((uint32_t)cap) & WL_SEAT_CAPABILITY_POINTER;

        if (HAS_KEYBOARD && !m_waylandState.keyboard) {
            m_waylandState.keyboard = makeShared<CCWlKeyboard>(m_waylandState.seat->sendGetKeyboard());

            m_waylandState.keyboard->setKeymap([this](CCWlKeyboard*, wl_keyboard_keymap_format format, int32_t fd, uint32_t size) {
                if (!m_waylandState.seatState.xkbContext)
                    return;

                if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
                    g_logger->log(HT_LOG_ERROR, "wayland: couldn't recognize keymap");
                    return;
                }

                const char* buf = (const char*)mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0);
                if (buf == MAP_FAILED) {
                    g_logger->log(HT_LOG_ERROR, "wayland: failed to mmap xkb keymap: {}", errno);
                    return;
                }

                m_waylandState.seatState.xkbKeymap =
                    xkb_keymap_new_from_buffer(m_waylandState.seatState.xkbContext, buf, size - 1, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

                munmap((void*)buf, size);
                close(fd);

                if (!m_waylandState.seatState.xkbKeymap) {
                    g_logger->log(HT_LOG_ERROR, "wayland: failed to compile xkb keymap");
                    return;
                }

                m_waylandState.seatState.xkbState = xkb_state_new(m_waylandState.seatState.xkbKeymap);
                if (!m_waylandState.seatState.xkbState) {
                    g_logger->log(HT_LOG_ERROR, "wayland: failed to create xkb state");
                    return;
                }

                const auto PCOMOPOSETABLE = xkb_compose_table_new_from_locale(m_waylandState.seatState.xkbContext, setlocale(LC_CTYPE, nullptr), XKB_COMPOSE_COMPILE_NO_FLAGS);

                if (!PCOMOPOSETABLE) {
                    g_logger->log(HT_LOG_ERROR, "wayland: failed to create xkb compose table");
                    return;
                }

                m_waylandState.seatState.xkbComposeState = xkb_compose_state_new(PCOMOPOSETABLE, XKB_COMPOSE_STATE_NO_FLAGS);
            });

            m_waylandState.keyboard->setModifiers([this](CCWlKeyboard* r, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
                if (!m_waylandState.seatState.xkbContext)
                    return;

                if (group != m_waylandState.seatState.currentLayer)
                    m_waylandState.seatState.currentLayer = group;

                xkb_state_update_mask(m_waylandState.seatState.xkbState, mods_depressed, mods_latched, mods_locked, 0, 0, group);

                m_currentMods       = 0;
                const auto XKB_MASK = mods_depressed | mods_latched | mods_locked;

#define GET_MOD_STATE(xkb, ht)                                                                                                                                                     \
    {                                                                                                                                                                              \
        auto idx = xkb_map_mod_get_index(m_waylandState.seatState.xkbKeymap, xkb);                                                                                                 \
        m_currentMods |= (XKB_MASK & (1 << idx)) ? ht : 0;                                                                                                                         \
    }

                GET_MOD_STATE(XKB_MOD_NAME_SHIFT, Input::HT_MODIFIER_SHIFT);
                GET_MOD_STATE(XKB_MOD_NAME_CAPS, Input::HT_MODIFIER_CAPS);
                GET_MOD_STATE(XKB_MOD_NAME_CTRL, Input::HT_MODIFIER_CTRL);
                GET_MOD_STATE(XKB_MOD_NAME_ALT, Input::HT_MODIFIER_ALT);
                GET_MOD_STATE(XKB_MOD_NAME_NUM, Input::HT_MODIFIER_MOD2);
                GET_MOD_STATE("Mod3", Input::HT_MODIFIER_MOD3);
                GET_MOD_STATE(XKB_MOD_NAME_LOGO, Input::HT_MODIFIER_META);
                GET_MOD_STATE("Mod5", Input::HT_MODIFIER_MOD5);

#undef GET_MOD_STATE
            });

            m_waylandState.keyboard->setKey([this](CCWlKeyboard* r, uint32_t serial, uint32_t time, uint32_t key, wl_keyboard_key_state state) { //
                onKey(key, state == WL_KEYBOARD_KEY_STATE_PRESSED);
            });

            m_waylandState.keyboard->setRepeatInfo([this](CCWlKeyboard* r, uint32_t rate, uint32_t delay) { //
                m_waylandState.seatState.repeatRate  = rate;
                m_waylandState.seatState.repeatDelay = delay;
            });

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
                m_currentMods     = 0;

                setCursor(HT_POINTER_ARROW);

                m_waylandState.seatState.pressedKeys.clear();
                stopRepeatTimer();
            });

            m_waylandState.pointer->setLeave([this](CCWlPointer* r, uint32_t serial, wl_proxy* surf) {
                auto w = windowForSurf(surf);

                if (!w)
                    return;

                w->mouseLeave();
                m_currentWindow.reset();
                m_currentMods = 0;

                m_waylandState.seatState.pressedKeys.clear();
                stopRepeatTimer();
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
        case HT_POINTER_TEXT: wlShape = WP_CURSOR_SHAPE_DEVICE_V1_SHAPE_TEXT; break;
        default: break;
    }

    m_waylandState.cursorShapeDev->sendSetShape(m_lastEnterSerial, wlShape);
}

void CWaylandPlatform::onKey(uint32_t keycode, bool state) {

    const auto HAS_IN_VEC = std::ranges::contains(m_waylandState.seatState.pressedKeys, keycode);

    if (state && HAS_IN_VEC) {
        g_logger->log(HT_LOG_ERROR, "Invalid key down event (key already pressed?)");
        return;
    } else if (!state && !HAS_IN_VEC) {
        g_logger->log(HT_LOG_ERROR, "Invalid key up event (stray release event?)");
        return;
    }

    if (state)
        m_waylandState.seatState.pressedKeys.emplace_back(keycode);
    else
        std::erase(m_waylandState.seatState.pressedKeys, keycode);

    if (!m_currentWindow)
        return;

    Input::SKeyboardKeyEvent e;
    e.down    = state;
    e.modMask = m_currentMods;

    if (state) {
        const auto SYM = xkb_state_key_get_one_sym(m_waylandState.seatState.xkbState, keycode + 8);

        if (SYM == XKB_KEY_Left || SYM == XKB_KEY_Right || SYM == XKB_KEY_Up || SYM == XKB_KEY_Down) {
            // skip compose
            e.xkbKeysym = SYM;
            m_currentWindow->keyboardKey(e);
            m_waylandState.seatState.repeatKeyEvent = e;
            startRepeatTimer();
            return;
        }

        enum xkb_compose_status composeStatus = XKB_COMPOSE_NOTHING;
        if (m_waylandState.seatState.xkbComposeState) {
            xkb_compose_state_feed(m_waylandState.seatState.xkbComposeState, SYM);
            composeStatus = xkb_compose_state_get_status(m_waylandState.seatState.xkbComposeState);
        }

        const bool COMPOSED = composeStatus == XKB_COMPOSE_COMPOSED;

        char       buf[16] = {0};
        int        len     = COMPOSED ? xkb_compose_state_get_utf8(m_waylandState.seatState.xkbComposeState, buf, sizeof(buf)) /* nullbyte */ + 1 :
                                        xkb_keysym_to_utf8(SYM, buf, sizeof(buf)) /* already includes a nullbyte */;

        if (len > 1) {
            e.xkbKeysym = SYM;
            e.utf8      = std::string{buf, sc<size_t>(len - 1)};
            m_currentWindow->keyboardKey(e);
            m_waylandState.seatState.repeatKeyEvent = e;
        }

        startRepeatTimer();

        return;

    } else if (m_waylandState.seatState.xkbComposeState && xkb_compose_state_get_status(m_waylandState.seatState.xkbComposeState) == XKB_COMPOSE_COMPOSED)
        xkb_compose_state_reset(m_waylandState.seatState.xkbComposeState);

    m_waylandState.seatState.repeatKeyEvent = e;
    m_currentWindow->keyboardKey(e);
    stopRepeatTimer();
}

void CWaylandPlatform::onRepeatTimerFire() {
    if (!m_currentWindow)
        return;

    m_currentWindow->keyboardKey(m_waylandState.seatState.repeatKeyEvent);

    // add a repeat timer
    m_waylandState.seatState.repeatTimer =
        g_backend->addTimer(std::chrono::milliseconds(1000 / m_waylandState.seatState.repeatRate), [this](ASP<CTimer> self, void*) { onRepeatTimerFire(); }, nullptr);
}

void CWaylandPlatform::startRepeatTimer() {
    if (m_waylandState.seatState.repeatDelay == 0 || m_waylandState.seatState.repeatRate == 0)
        return;

    if (m_waylandState.seatState.repeatTimer)
        m_waylandState.seatState.repeatTimer->cancel();

    m_waylandState.seatState.repeatKeyEvent.repeat = true;

    m_waylandState.seatState.repeatTimer =
        g_backend->addTimer(std::chrono::milliseconds(m_waylandState.seatState.repeatDelay), [this](ASP<CTimer> self, void*) { onRepeatTimerFire(); }, nullptr);
}

void CWaylandPlatform::stopRepeatTimer() {
    if (m_waylandState.seatState.repeatTimer)
        m_waylandState.seatState.repeatTimer->cancel();
    m_waylandState.seatState.repeatTimer.reset();
}

SP<CWaylandSessionLockState> CWaylandPlatform::aquireSessionLock() {
    if (m_sessionLockState)
        return m_sessionLockState.lock();

    auto sessionLock = makeShared<CWaylandSessionLockState>(makeShared<CCExtSessionLockV1>(m_waylandState.sessionLock->sendLock()));

    // roundtrip in case the compositor sends `finished` right away
    wl_display_roundtrip(m_waylandState.display);

    m_sessionLockState = sessionLock;

    return sessionLock;
}
