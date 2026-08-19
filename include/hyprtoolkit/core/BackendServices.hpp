#pragma once

#include <chrono>
#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <vector>

#include <hyprutils/math/Box.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/signal/Signal.hpp>

#include "../types/PointerShape.hpp"
#include "SessionLock.hpp"

namespace Hyprtoolkit {
    class CTimer;
    class IOutput;

    using TimerDuration     = std::chrono::steady_clock::duration;
    using EmbeddedSurfaceID = uint64_t;

    class IEventLoop {
      public:
        virtual ~IEventLoop() = default;

        // All callbacks must execute serially on the thread that owns toolkit
        // surfaces. The host retains callbacks until they run or are removed.
        virtual void addFd(int fd, std::function<void()>&& callback) = 0;
        virtual void removeFd(int fd)                                = 0;
        virtual Hyprutils::Memory::CAtomicSharedPointer<CTimer>
        addTimer(const TimerDuration& timeout, std::function<void(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self, void* data)> callback, void* data, bool force = false) = 0;
        virtual void addIdle(const std::function<void()>& callback)                                                                                                            = 0;
        virtual void cancelPending()                                                                                                                                           = 0;

        // Embedded loops are already running and may implement this as a no-op.
        virtual void enterLoop() = 0;
    };

    class IClipboard {
      public:
        virtual ~IClipboard() = default;

        virtual void        setText(const std::string& text) = 0;
        virtual std::string getText()                        = 0;
    };

    class ITextInput {
      public:
        virtual ~ITextInput() = default;

        // surface is zero for native backend windows.
        virtual void activate(EmbeddedSurfaceID surface, const Hyprutils::Math::CBox& cursorBox, const std::string& surroundingText, size_t cursor) = 0;
        virtual void deactivate(EmbeddedSurfaceID surface)                                                                                          = 0;
    };

    class ICursor {
      public:
        virtual ~ICursor() = default;

        // surface is zero for native backend windows.
        virtual void setShape(EmbeddedSurfaceID surface, ePointerShape shape) = 0;
    };

    class IOutputProvider {
      public:
        virtual ~IOutputProvider() = default;

        virtual std::vector<Hyprutils::Memory::CSharedPointer<IOutput>> outputs() = 0;

        struct {
            Hyprutils::Signal::CSignalT<Hyprutils::Memory::CSharedPointer<IOutput>> outputAdded;
        } m_events;
    };

    class ISessionLockProvider {
      public:
        virtual ~ISessionLockProvider() = default;

        virtual std::expected<Hyprutils::Memory::CSharedPointer<ISessionLockState>, eSessionLockError> acquire() = 0;
    };
}
