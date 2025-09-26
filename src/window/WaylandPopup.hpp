#pragma once

#include "WaylandWindow.hpp"

namespace Hyprtoolkit {

    // TODO: de-dup this shit
    class CWaylandPopup : public IToolkitWindow {
      public:
        CWaylandPopup(const SPopupCreationData& data, SP<CWaylandWindow> parent);
        ~CWaylandPopup();

        virtual Hyprutils::Math::Vector2D                  pixelSize();
        virtual float                                      scale();
        virtual void                                       render();
        virtual void                                       setCursor(ePointerShape shape);
        virtual void                                       close();
        virtual void                                       open();
        virtual Hyprutils::Memory::CSharedPointer<IWindow> openPopup(const SPopupCreationData& data);

      private:
        float m_fractionalScale = 1.0;

        bool  m_open = false;

        struct {
            SP<CCWlSurface>                   surface;
            SP<CCXdgSurface>                  xdgSurface;
            SP<CCXdgPopup>                    xdgPopup;
            SP<CCXdgPositioner>               xdgPositioner;
            SP<CCWlCallback>                  frameCallback;

            std::array<SP<CWaylandBuffer>, 2> wlBuffers;
            SP<Aquamarine::CSwapchain>        swapchain;
            size_t                            bufIdx = 0;

            Hyprutils::Math::Vector2D         size;
            Hyprutils::Math::Vector2D         logicalSize;
            float                             appliedScale;
            SP<CCWpFractionalScaleV1>         fractional = nullptr;
            SP<CCWpViewport>                  viewport   = nullptr;
            uint32_t                          serial     = 0;
            bool                              grabbed    = false;
        } m_waylandState;

        WP<CWaylandWindow>                    m_parent;

        std::chrono::steady_clock::time_point m_lastFrame = std::chrono::steady_clock::now();

        void                                  onCallback();
        void                                  onScaleUpdate();
        void                                  configure(const Hyprutils::Math::Vector2D& size, uint32_t serial);
        void                                  resizeSwapchain(const Hyprutils::Math::Vector2D& pixelSize);

        SPopupCreationData                    m_creationData;

        friend class CWaylandPlatform;
        friend class CWaylandWindow;
    };
};