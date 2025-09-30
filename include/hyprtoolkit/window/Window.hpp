#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include "WindowTypes.hpp"

namespace Hyprtoolkit {
    class IElement;

    class IWindow {
      public:
        virtual ~IWindow() = default;

        virtual Hyprutils::Math::Vector2D                  pixelSize()                               = 0;
        virtual float                                      scale()                                   = 0;
        virtual void                                       close()                                   = 0;
        virtual void                                       open()                                    = 0;
        virtual Hyprutils::Memory::CSharedPointer<IWindow> openPopup(const SPopupCreationData& data) = 0;

        struct {
            // coordinates here are logical, meaning pixel size is this * scale()
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> resized;

            // user requested a close.
            Hyprutils::Signal::CSignalT<> closeRequest;

            // popup closed
            Hyprutils::Signal::CSignalT<> popupClosed;
        } m_events;

        Hyprutils::Memory::CSharedPointer<IElement> m_rootElement;

      private:
        IWindow() = default;

        friend class CWaylandWindow;
        friend class IToolkitWindow;
    };
};
