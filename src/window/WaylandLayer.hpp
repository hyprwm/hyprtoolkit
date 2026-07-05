#pragma once

#include "IWaylandWindow.hpp"
#include <wlr-layer-shell-unstable-v1.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {

    class CWaylandLayer : public IWaylandWindow {
      public:
        CWaylandLayer(const SWindowCreationData& data);
        virtual ~CWaylandLayer();

        virtual void close();
        virtual void open();
        virtual void render();
        virtual void setSize(const Hyprutils::Math::Vector2D& size);

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
