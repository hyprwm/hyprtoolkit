#pragma once

#include "IWaylandWindow.hpp"
#include <wlr-layer-shell-unstable-v1.hpp>

namespace Hyprtoolkit {

    class CWaylandLayer : public IWaylandWindow {
      public:
        CWaylandLayer(const SWindowCreationData& data);
        virtual ~CWaylandLayer();

        virtual void close();
        virtual void open();
        virtual void render();

      private:
        SWindowCreationData m_creationData;

        struct {
            SP<CCZwlrLayerSurfaceV1> layerSurface;
            bool                     configured = false;
        } m_layerState;

        friend class CWaylandPlatform;
        friend class CWaylandPopup;
    };
};