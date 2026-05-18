#pragma once

#include "IWaylandWindow.hpp"
namespace Hyprtoolkit {

    class CWaylandWindow : public IWaylandWindow {
      public:
        CWaylandWindow(const SWindowCreationData& data);
        virtual ~CWaylandWindow();

        virtual void close();
        virtual void open();

        virtual void setSize(const Hyprutils::Math::Vector2D& size);
        virtual void startInteractiveResize(eResizeEdge edges);

        virtual void mouseMove(const Hyprutils::Math::Vector2D& local);
        virtual void mouseButton(const Input::eMouseButton button, bool state);

        virtual void onPreRender();

      private:
        SWindowCreationData m_creationData;

        // pixel proximity (logical) that triggers an edge-resize cursor/grab
        static constexpr int kResizeBorderPx = 6;
        eResizeEdge          edgeForPos(const Hyprutils::Math::Vector2D& local) const;

        // setSize pins min == max == requested temporarily; the next configure restores
        // the user's original constraints from m_creationData.
        struct {
            bool                      pending = false;
            Hyprutils::Math::Vector2D size;
        } m_pendingResize;
        void restoreUserSizeConstraints();

        // active xdg_toplevel states from the latest configure. autosize is suppressed
        // when any of these constrain the compositor-decided size.
        bool m_isMaximized  = false;
        bool m_isFullscreen = false;
        bool m_isResizing   = false;
        bool m_isTiled      = false;

        friend class CWaylandPlatform;
        friend class CWaylandPopup;
    };
};