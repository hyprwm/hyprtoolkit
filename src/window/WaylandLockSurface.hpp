#pragma once

#include "IWaylandWindow.hpp"
#include "../lock/WaylandSessionLock.hpp"
#include <ext-session-lock-v1.hpp>

namespace Hyprtoolkit {
    class CWaylandLockSurface : public IWaylandWindow {
      public:
        CWaylandLockSurface(const SWindowCreationData& data);
        virtual ~CWaylandLockSurface();

        virtual void close();
        virtual void open();
        virtual void render();

      private:
        uint32_t m_outputHandle = 0;

        struct {
            SP<CCExtSessionLockSurfaceV1> lockSurface;
            bool                          configured = false;
        } m_lockSurfaceState;

        friend class CWaylandSessionLockState;
    };
};
