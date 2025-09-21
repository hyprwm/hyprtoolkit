#pragma once

#include "../../helpers/Memory.hpp"

#include <vector>
#include <functional>

#include <wayland-client.h>

#include <wayland.hpp>
#include <xdg-shell.hpp>
#include <linux-dmabuf-v1.hpp>
#include <fractional-scale-v1.hpp>
#include <viewporter.hpp>

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

        bool               dispatchEvents();

        SP<CWaylandWindow> windowForSurf(wl_proxy* proxy);

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

            // control
            bool dmabufFailed = false;
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
