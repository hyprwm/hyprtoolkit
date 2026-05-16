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

      private:
        SWindowCreationData m_creationData;

        friend class CWaylandPlatform;
        friend class CWaylandPopup;
    };
};