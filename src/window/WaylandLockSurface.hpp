#pragma once

#include "IWaylandWindow.hpp"
#include <ext-session-lock-v1.hpp>

namespace Hyprtoolkit {
    class CWaylandLockSurface : public IWaylandWindow {
      public:
        CWaylandLockSurface(const SWindowCreationData& data, const SP<CCExtSessionLockV1>& lockObject);
        virtual ~CWaylandLockSurface();

        virtual void close();
        virtual void open();
        virtual void render();

      private:
        uint32_t               m_outputId = 0;
        WP<CCExtSessionLockV1> m_lockObject;

        struct {
            SP<CCExtSessionLockSurfaceV1> lockSurface;
            bool                          configured = false;
        } m_lockSurfaceState;

        friend class CWaylandPlatform;
    };
};
