#pragma once

#include "IWaylandWindow.hpp"

namespace Hyprtoolkit {

    class CWaylandWindow;

    // TODO: de-dup this shit
    class CWaylandPopup : public IWaylandWindow {
      public:
        CWaylandPopup(const SWindowCreationData& data, SP<CWaylandWindow> parent);
        virtual ~CWaylandPopup();

        virtual void                                       close();
        virtual void                                       open();
        virtual Hyprutils::Memory::CSharedPointer<IWindow> openPopup(const SWindowCreationData& data);

      private:
        WP<CWaylandWindow>  m_parent;
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