#pragma once

#include <hyprutils/signal/Signal.hpp>

namespace Hyprtoolkit {
    class IOutput {
      public:
        virtual ~IOutput()        = default;
        virtual uint32_t handle() = 0;

        struct {
            // output removed
            Hyprutils::Signal::CSignalT<> removed;
        } m_events;
    };
}
