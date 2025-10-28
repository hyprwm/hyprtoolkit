#pragma once

#include <hyprtoolkit/types/PointerShape.hpp>
#include <hyprtoolkit/core/Input.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/core/SessionLock.hpp>

#include "../../helpers/Memory.hpp"

#include <vector>
#include <functional>

#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-compose.h>

#include <wayland-client.h>

#include <wayland.hpp>
#include <xdg-shell.hpp>
#include <linux-dmabuf-v1.hpp>
#include <fractional-scale-v1.hpp>
#include <viewporter.hpp>
#include <cursor-shape-v1.hpp>
#include <text-input-unstable-v3.hpp>
#include <wlr-layer-shell-unstable-v1.hpp>
#include <linux-drm-syncobj-v1.hpp>
#include <ext-session-lock-v1.hpp>

#include <aquamarine/allocator/GBM.hpp>
#include <aquamarine/backend/Misc.hpp>

namespace Hyprtoolkit {
    typedef std::function<void(void)> FIdleCallback;

    class CWaylandWindow;
    class IWaylandWindow;
    class CWaylandLayer;
    class CWaylandOutput;
    class CWaylandSessionLockState;

    class CWaylandPlatform {
      public:
        CWaylandPlatform() = default;
        ~CWaylandPlatform();

        bool                         attempt();

        void                         initSeat();
        void                         initShell();
        bool                         initDmabuf();
        void                         initIM();
        void                         setCursor(ePointerShape shape);

        bool                         dispatchEvents();

        SP<IWaylandWindow>           windowForSurf(wl_proxy* proxy);
        WP<CWaylandOutput>           outputForHandle(uint32_t handle);

        void                         onKey(uint32_t keycode, bool state);
        void                         startRepeatTimer();
        void                         stopRepeatTimer();

        void                         onRepeatTimerFire();

        SP<CWaylandSessionLockState> aquireSessionLock();

        // dmabuf formats
        std::vector<Aquamarine::SDRMFormat> m_dmabufFormats;

        SP<Aquamarine::CGBMAllocator>       m_allocator;

        struct {
            wl_display* display = nullptr;

            // hw-s types
            Hyprutils::Memory::CSharedPointer<CCWlRegistry>                 registry;
            Hyprutils::Memory::CSharedPointer<CCWlSeat>                     seat;
            Hyprutils::Memory::CSharedPointer<CCWlShm>                      shm;
            Hyprutils::Memory::CSharedPointer<CCXdgWmBase>                  xdg;
            Hyprutils::Memory::CSharedPointer<CCWlCompositor>               compositor;
            Hyprutils::Memory::CSharedPointer<CCZwpLinuxDmabufV1>           dmabuf;
            Hyprutils::Memory::CSharedPointer<CCZwpLinuxDmabufFeedbackV1>   dmabufFeedback;
            Hyprutils::Memory::CSharedPointer<CCWpFractionalScaleManagerV1> fractional;
            Hyprutils::Memory::CSharedPointer<CCWpViewporter>               viewporter;
            Hyprutils::Memory::CSharedPointer<CCWlKeyboard>                 keyboard;
            Hyprutils::Memory::CSharedPointer<CCWlPointer>                  pointer;
            Hyprutils::Memory::CSharedPointer<CCWpCursorShapeManagerV1>     cursorShapeMgr;
            Hyprutils::Memory::CSharedPointer<CCWpCursorShapeDeviceV1>      cursorShapeDev;
            Hyprutils::Memory::CSharedPointer<CCZwpTextInputManagerV3>      textInputManager;
            Hyprutils::Memory::CSharedPointer<CCZwpTextInputV3>             textInput;
            Hyprutils::Memory::CSharedPointer<CCZwlrLayerShellV1>           layerShell;
            Hyprutils::Memory::CSharedPointer<CCWpLinuxDrmSyncobjManagerV1> syncobj;
            Hyprutils::Memory::CSharedPointer<CCExtSessionLockManagerV1>    sessionLock;

            // control
            bool initialized  = false;
            bool dmabufFailed = false;

            struct {
                xkb_context*             xkbContext      = nullptr;
                xkb_keymap*              xkbKeymap       = nullptr;
                xkb_state*               xkbState        = nullptr;
                xkb_compose_state*       xkbComposeState = nullptr;
                uint32_t                 currentLayer    = 0;
                uint32_t                 repeatRate = 10, repeatDelay = 500;
                std::vector<uint32_t>    pressedKeys;
                Input::SKeyboardKeyEvent repeatKeyEvent;
                ASP<CTimer>              repeatTimer;
            } seatState;

            struct {
                bool        entered = false, enabled = false;
                std::string preeditString;
                int         preeditBegin = 0, preeditEnd = 0;
                std::string commitString;
                size_t      deleteBefore = 0, deleteAfter = 0;

                std::string originalString;
            } imState;
        } m_waylandState;

        struct {
            int         fd       = -1;
            std::string nodeName = "";
        } m_drmState;

        std::vector<SP<CWaylandOutput>> m_outputs;

        std::vector<WP<CWaylandWindow>> m_windows;
        std::vector<WP<CWaylandLayer>>  m_layers;
        WP<IWaylandWindow>              m_currentWindow;
        uint32_t                        m_currentMods     = 0; // HT modifiers, not xkb
        uint32_t                        m_lastEnterSerial = 0;

        WP<CWaylandSessionLockState>    m_sessionLockState;
    };

    inline UP<CWaylandPlatform> g_waylandPlatform;
}
