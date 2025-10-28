#pragma once

#include "wayland.hpp"

#include "../helpers/Memory.hpp"
#include <hyprtoolkit/core/SystemLock.hpp>
#include <ext-session-lock-v1.hpp>

namespace Hyprtoolkit {
    class CWaylandLockSurface;

    class CWaylandSessionLockState : public ISessionLockState {
      public:
        CWaylandSessionLockState(SP<CCExtSessionLockV1> lock);
        ~CWaylandSessionLockState() = default;

        virtual void                         unlock();

        void                                 onOutputRemoved(uint32_t outputHandle);

        std::vector<WP<CWaylandLockSurface>> m_lockSurfaces;
        SP<CCExtSessionLockV1>               m_lock            = nullptr;
        bool                                 m_sessionLocked   = false;
        bool                                 m_sessionUnlocked = false;
        bool                                 m_denied          = false; // set on the finished event
    };
}
