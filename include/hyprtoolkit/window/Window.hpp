#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

#include "../core/Input.hpp"

namespace Hyprtoolkit {
    class IElement;
    class IWindow;
    class IOutput;
    struct SWindowCreationData;

    enum eWindowType : uint8_t {
        HT_WINDOW_TOPLEVEL     = 0,
        HT_WINDOW_POPUP        = 1,
        HT_WINDOW_LAYER        = 2,
        HT_WINDOW_LOCK_SURFACE = 3,
    };

    // matches xdg_toplevel resize edges; combine with bitwise or for corners
    enum eResizeEdge : uint32_t {
        HT_RESIZE_EDGE_NONE         = 0,
        HT_RESIZE_EDGE_TOP          = 1,
        HT_RESIZE_EDGE_BOTTOM       = 2,
        HT_RESIZE_EDGE_LEFT         = 4,
        HT_RESIZE_EDGE_TOP_LEFT     = 5,
        HT_RESIZE_EDGE_BOTTOM_LEFT  = 6,
        HT_RESIZE_EDGE_RIGHT        = 8,
        HT_RESIZE_EDGE_TOP_RIGHT    = 9,
        HT_RESIZE_EDGE_BOTTOM_RIGHT = 10,
    };

    class CWindowBuilder {
      public:
        ~CWindowBuilder() = default;

        static Hyprutils::Memory::CSharedPointer<CWindowBuilder> begin();
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        type(eWindowType);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        appTitle(std::string&&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        appClass(std::string&&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        preferredSize(const Hyprutils::Math::Vector2D&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        minSize(const Hyprutils::Math::Vector2D&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder>        maxSize(const Hyprutils::Math::Vector2D&);

        // only for LAYER and LOCK_SURFACE
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> prefferedOutput(const Hyprutils::Memory::CSharedPointer<IOutput>& output);

        // only for HT_WINDOW_LAYER
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> marginTopLeft(const Hyprutils::Math::Vector2D&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> marginBottomRight(const Hyprutils::Math::Vector2D&);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> layer(uint32_t);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> anchor(uint32_t);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> exclusiveEdge(uint32_t);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> exclusiveZone(int32_t);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> kbInteractive(uint32_t);

        // only for HT_WINDOW_POPUP
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> parent(const Hyprutils::Memory::CSharedPointer<IWindow>& parent);
        Hyprutils::Memory::CSharedPointer<CWindowBuilder> pos(const Hyprutils::Math::Vector2D&);

        Hyprutils::Memory::CSharedPointer<IWindow>        commence();

      private:
        Hyprutils::Memory::CWeakPointer<CWindowBuilder>        m_self;
        Hyprutils::Memory::CUniquePointer<SWindowCreationData> m_data;

        CWindowBuilder() = default;
    };

    class IWindow {
      public:
        virtual ~IWindow() = default;

        virtual Hyprutils::Math::Vector2D pixelSize() = 0;
        virtual float                     scale()     = 0;
        virtual void                      close()     = 0;
        virtual void                      open()      = 0;
        virtual Hyprutils::Math::Vector2D cursorPos() = 0;

        // request a new logical size. advisory only: the compositor has final
        // authority over xdg_toplevel size and may ignore, clamp, or override.
        // listen on m_events.resized for the size the compositor actually applied.
        // no-op for layer, lock, popup.
        virtual void                      setSize(const Hyprutils::Math::Vector2D& size) {}

        // hand off to the compositor for an interactive edge/corner resize.
        // must be called from a pointer-press handler so the latest button
        // serial is fresh. no-op for layer, lock, popup.
        virtual void                      startInteractiveResize(eResizeEdge edges) {}

        struct {
            // coordinates here are logical, meaning pixel size is this * scale()
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> resized;

            // user requested a close.
            Hyprutils::Signal::CSignalT<> closeRequest;

            // popup closed
            Hyprutils::Signal::CSignalT<> popupClosed;

            // layer closed
            Hyprutils::Signal::CSignalT<> layerClosed;

            // (global) key events
            Hyprutils::Signal::CSignalT<Input::SKeyboardKeyEvent> keyboardKey;
        } m_events;

        Hyprutils::Memory::CSharedPointer<IElement> m_rootElement;

        HT_HIDDEN : IWindow() = default;
    };
};
