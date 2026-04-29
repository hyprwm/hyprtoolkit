#pragma once

#include "IWaylandWindow.hpp"

namespace Hyprtoolkit {

    class CWaylandPopup : public IWaylandWindow {
      public:
        CWaylandPopup(const SWindowCreationData& data, SP<IWaylandWindow> parent);
        virtual ~CWaylandPopup();

        virtual void close();
        virtual void open();

      private:
        WP<IWaylandWindow>  m_parent;
        SWindowCreationData m_creationData;

        struct {
            SP<CCXdgPositioner> xdgPositioner;
            SP<CCXdgPopup>      xdgPopup;
            bool                grabbed = false;
        } m_wlPopupState;

        friend class CWaylandPlatform;
        friend class CWaylandWindow;
    };
};