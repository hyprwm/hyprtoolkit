#pragma once

#include <hyprtoolkit/types/PointerShape.hpp>

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

#include <aquamarine/allocator/GBM.hpp>
#include <aquamarine/backend/Misc.hpp>

namespace Hyprtoolkit {
    typedef std::function<void(void)> FIdleCallback;

    class CWaylandWindow;

    class CWaylandPlatform {
      public:
        CWaylandPlatform() = default;
        ~CWaylandPlatform();

        bool               attempt();

        void               initSeat();
        void               initShell();
        bool               initDmabuf();
        void               initIM();
        void               setCursor(ePointerShape shape);

        bool               dispatchEvents();

        SP<CWaylandWindow> windowForSurf(wl_proxy* proxy);

        void               onKey(uint32_t keycode, bool state);

        //
        std::vector<FIdleCallback> m_idleCallbacks;

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

            // control
            bool dmabufFailed = false;

            struct {
                xkb_context*          xkbContext      = nullptr;
                xkb_keymap*           xkbKeymap       = nullptr;
                xkb_state*            xkbState        = nullptr;
                xkb_compose_state*    xkbComposeState = nullptr;
                uint32_t              currentLayer    = 0;
                std::vector<uint32_t> pressedKeys;
            } seatState;

            struct {
                bool        entered = false, enabled = false;
                std::string preeditString;
                int         preeditBegin = 0, preeditEnd = 0;
                std::string commitString;
                size_t      deleteBefore = 0, deleteAfter = 0;
            } imState;
        } m_waylandState;

        struct {
            int         fd       = -1;
            std::string nodeName = "";
        } m_drmState;

        std::vector<WP<CWaylandWindow>> m_windows;
        WP<CWaylandWindow>              m_currentWindow;
        uint32_t                        m_lastEnterSerial = 0;
    };

    inline UP<CWaylandPlatform> g_waylandPlatform;
}
