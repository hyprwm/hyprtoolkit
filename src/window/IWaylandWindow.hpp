#pragma once

#include <hyprtoolkit/window/Window.hpp>

#include <hyprutils/math/Vector2D.hpp>

#include <aquamarine/allocator/Swapchain.hpp>
#include <aquamarine/allocator/GBM.hpp>

#include "../helpers/Memory.hpp"
#include "ToolkitWindow.hpp"

#include <wayland.hpp>
#include <xdg-shell.hpp>
#include <fractional-scale-v1.hpp>
#include <viewporter.hpp>
#include <text-input-unstable-v3.hpp>

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

    class IWaylandWindow : public IToolkitWindow {
      public:
        virtual ~IWaylandWindow() = default;

        virtual Hyprutils::Math::Vector2D pixelSize();
        virtual float                     scale();
        virtual void                      render();
        virtual void                      setCursor(ePointerShape shape);
        virtual SP<IWindow>               openPopup(const SWindowCreationData& data);
        virtual void                      mouseMove(const Hyprutils::Math::Vector2D& local);
        virtual void                      mouseButton(const Input::eMouseButton button, bool state);
        virtual void                      mouseAxis(const Input::eAxisAxis axis, float delta);
        virtual void                      setIMTo(const Hyprutils::Math::CBox& box, const std::string& str, size_t cursor);
        virtual void                      resetIM();
        virtual void                      onPreRender();

        std::vector<WP<IWindow>>          m_popups;

      protected:
        virtual void onCallback();
        virtual void onScaleUpdate();
        virtual void configure(const Hyprutils::Math::Vector2D& size, uint32_t serial);
        virtual void resizeSwapchain(const Hyprutils::Math::Vector2D& pixelSize);

        float        m_fractionalScale = 1.0;

        bool         m_open                  = false;
        bool         m_ignoreNextButtonEvent = false;

        struct {
            SP<CCWlSurface>                         surface;
            SP<CCXdgSurface>                        xdgSurface;
            SP<CCXdgToplevel>                       xdgToplevel;
            SP<CCWlCallback>                        frameCallback;

            std::array<SP<CWaylandBuffer>, 2>       wlBuffers;
            SP<Aquamarine::CSwapchain>              swapchain;
            size_t                                  bufIdx = 0;

            Hyprutils::Math::Vector2D               size;
            Hyprutils::Math::Vector2D               logicalSize;
            float                                   appliedScale;
            SP<CCWpFractionalScaleV1>               fractional = nullptr;
            SP<CCWpViewport>                        viewport   = nullptr;
            uint32_t                                serial     = 0;

            std::optional<Hyprutils::Math::CRegion> lastOpaqueRegion;
        } m_waylandState;

        std::chrono::steady_clock::time_point m_lastFrame = std::chrono::steady_clock::now();

        friend class CWaylandPlatform;
        friend class CWaylandPopup;
        friend class CWaylandWindow;
    };

}
