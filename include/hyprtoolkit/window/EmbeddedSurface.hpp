#pragma once

#include <cstdint>
#include <string>

#include <hyprutils/math/Region.hpp>
#include <hyprutils/math/Vector2D.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>

#include "../core/Input.hpp"
#include "../core/BackendServices.hpp"
#include "../types/PointerShape.hpp"

namespace Hyprtoolkit {
    class IElement;

    class IEmbeddedSurface {
      public:
        virtual ~IEmbeddedSurface() = default;

        virtual void resize(const Hyprutils::Math::Vector2D& logicalSize, const Hyprutils::Math::Vector2D& pixelSize, float scale) = 0;

        // Every method must be called serially on the toolkit event-loop
        // thread. The GLES3 context and destination draw framebuffer must be
        // current for render(). Rendering uses premultiplied alpha and restores
        // modified GL state. A buffer age of zero requests a full redraw.
        virtual void render(uint32_t bufferAge = 1) = 0;

        virtual void pointerEnter(const Hyprutils::Math::Vector2D& local)    = 0;
        virtual void pointerMotion(const Hyprutils::Math::Vector2D& local)   = 0;
        virtual void pointerButton(Input::eMouseButton button, bool pressed) = 0;
        virtual void pointerAxis(Input::eAxisAxis axis, float delta)         = 0;
        virtual void pointerLeave()                                          = 0;
        virtual void keyboardEnter()                                         = 0;
        virtual void keyboardKey(const Input::SKeyboardKeyEvent& event)      = 0;
        virtual void keyboardLeave()                                         = 0;
        virtual void imCommit(const std::string& text)                       = 0;
        virtual void imApply()                                               = 0;
        // A primary touch without a touch-aware target emulates a captured
        // left-button tap on release. Drag controls should consume touch events.
        virtual void                                        touchDown(int32_t id, const Hyprutils::Math::Vector2D& local, uint32_t timeMs)   = 0;
        virtual void                                        touchMotion(int32_t id, const Hyprutils::Math::Vector2D& local, uint32_t timeMs) = 0;
        virtual void                                        touchUp(int32_t id, uint32_t timeMs)                                             = 0;
        virtual void                                        touchCancel(int32_t id, uint32_t timeMs)                                         = 0;

        virtual Hyprutils::Memory::CSharedPointer<IElement> rootElement() = 0;
        virtual EmbeddedSurfaceID                           id()          = 0;

        struct {
            Hyprutils::Signal::CSignalT<>                         frameRequested;
            Hyprutils::Signal::CSignalT<Hyprutils::Math::CRegion> damaged;
            Hyprutils::Signal::CSignalT<ePointerShape>            cursorChanged;
        } m_events;
    };
}
