#pragma once

#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class IOutput {
      public:
        virtual ~IOutput()           = default;
        virtual uint32_t    handle() = 0;
        virtual std::string port()   = 0;
        virtual std::string desc()   = 0;

        struct {
            /* output removed */
            Hyprutils::Signal::CSignalT<> removed;
        } m_events;
    };
}
