#pragma once

#include <hyprtoolkit/window/Window.hpp>
#include <hyprtoolkit/window/WindowTypes.hpp>

#include <hyprutils/math/Vector2D.hpp>

#include <aquamarine/allocator/Swapchain.hpp>
#include <aquamarine/allocator/GBM.hpp>

#include "../helpers/Memory.hpp"
#include "ToolkitWindow.hpp"

#include <wayland.hpp>
#include <xdg-shell.hpp>
#include <fractional-scale-v1.hpp>
#include <viewporter.hpp>

#include <wayland-egl.h>
#include "../core/renderPlatforms/Egl.hpp"

#include <chrono>

namespace Hyprtoolkit {
    class CWaylandBuffer {
      public:
        CWaylandBuffer(Hyprutils::Memory::CSharedPointer<Aquamarine::IBuffer> buffer);
        ~CWaylandBuffer();
        bool good();

        bool pendingRelease = false;

        struct {
            Hyprutils::Memory::CSharedPointer<CCWlBuffer> buffer;
        } m_waylandState;

        Hyprutils::Memory::CWeakPointer<Aquamarine::IBuffer> m_buffer;
    };

    class CWaylandWindow : public IToolkitWindow {
      public:
        CWaylandWindow(const SWindowCreationData& data);
        ~CWaylandWindow();

        virtual Hyprutils::Math::Vector2D pixelSize();
        virtual float                     scale();
        virtual void                      render();
        virtual void                      setCursor(ePointerShape shape);
        virtual void                      close();
        virtual void                      open();

      private:
        float m_fractionalScale = 1.0;

        bool  m_open = false;

        struct {
            SP<CCWlSurface>   surface;
            SP<CCXdgSurface>  xdgSurface;
            SP<CCXdgToplevel> xdgToplevel;
            SP<CCWlCallback>  frameCallback;

            // FIXME: consider using more raw GLES3.2 for this, instead of wl_egl.
            // std::array<SP<CWaylandBuffer>, 2> wlBuffers;
            // SP<Aquamarine::CSwapchain>        swapchain;

            wl_egl_window*            eglWindow = nullptr;
            Hyprutils::Math::Vector2D size;
            Hyprutils::Math::Vector2D logicalSize;
            float                     appliedScale;
            EGLSurface                eglSurface = nullptr;
            SP<CCWpFractionalScaleV1> fractional = nullptr;
            SP<CCWpViewport>          viewport   = nullptr;
            uint32_t                  serial     = 0;
        } m_waylandState;

        std::chrono::steady_clock::time_point m_lastFrame = std::chrono::steady_clock::now();

        void                                  onCallback();
        void                                  onScaleUpdate();
        void                                  configure(const Hyprutils::Math::Vector2D& size, uint32_t serial);
        void                                  resizeSwapchain(const Hyprutils::Math::Vector2D& pixelSize);

        SWindowCreationData                   m_creationData;

        friend class CWaylandPlatform;
    };
};