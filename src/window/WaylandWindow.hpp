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

      private:
        SWindowCreationData m_creationData;

        // pixel proximity (logical) that triggers an edge-resize cursor/grab
        static constexpr int kResizeBorderPx = 6;
        eResizeEdge          edgeForPos(const Hyprutils::Math::Vector2D& local) const;

        friend class CWaylandPlatform;
        friend class CWaylandPopup;
    };
};