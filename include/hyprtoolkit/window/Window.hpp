#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class IElement;

    class IWindow {
      public:
        virtual ~IWindow() = default;

        virtual Hyprutils::Math::Vector2D pixelSize() = 0;
        virtual float                     scale()     = 0;

        struct {
            Hyprutils::Signal::CSignalT<>                          opened;
            Hyprutils::Signal::CSignalT<>                          closed;
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> resized;
        } m_events;

        Hyprutils::Memory::CSharedPointer<IElement> m_rootElement;

      private:
        IWindow() = default;

        friend class CWaylandWindow;
        friend class IToolkitWindow;
    };
};
