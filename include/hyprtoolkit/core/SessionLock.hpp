#pragma once

#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    enum eSessionLockError : uint8_t {
        PLATFORM_UNINTIALIZED,
        DENIED,
    };

    class ISessionLockState {
      public:
        virtual ~ISessionLockState() = default;
        virtual void unlock()        = 0;

        struct {
            /* signals that we don't need to unlock anymore. It makes sense to exit upon recieving this */
            Hyprutils::Signal::CSignalT<> finished;
        } m_events;
    };
}
