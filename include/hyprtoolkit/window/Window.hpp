#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class IElement;
    class IWindow;
    struct SWindowCreationData;

    enum eWindowType : uint8_t {
        HT_WINDOW_TOPLEVEL = 0,
        HT_WINDOW_POPUP    = 1,
        HT_WINDOW_LAYER    = 2,
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

        struct {
            // coordinates here are logical, meaning pixel size is this * scale()
            Hyprutils::Signal::CSignalT<Hyprutils::Math::Vector2D> resized;

            // user requested a close.
            Hyprutils::Signal::CSignalT<> closeRequest;

            // popup closed
            Hyprutils::Signal::CSignalT<> popupClosed;

            // layer closed
            Hyprutils::Signal::CSignalT<> layerClosed;
        } m_events;

        Hyprutils::Memory::CSharedPointer<IElement> m_rootElement;

        HT_HIDDEN : IWindow() = default;
    };
};
