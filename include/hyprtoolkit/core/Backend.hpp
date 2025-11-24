#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <aquamarine/backend/Null.hpp>
#include <aquamarine/backend/Backend.hpp>
#include <functional>
#include <expected>
#include <string>
#include <sys/poll.h>

#include "LogTypes.hpp"
#include "SessionLock.hpp"
#include "../palette/Palette.hpp"

#include "CoreMacros.hpp"

namespace Hyprtoolkit {
    class IWindow;
    class IOutput;
    class CTimer;
    class ISystemIconFactory;
    struct SWindowCreationData;

    class IBackend {
      public:
        virtual ~IBackend();

        using LogFn = std::function<void(eLogLevel, const std::string&)>;

        /*
            Create a backend.
            There can only be one backend per process: In case of another create(),
            it will fail.
        */
        static Hyprutils::Memory::CSharedPointer<IBackend> create();

        /*
            Destroy the backend.
            Backend will be destroyed once:
             - All refs YOU hold are dead
             - You call this fn
        */
        virtual void destroy() = 0;

        virtual void setLogFn(LogFn&& fn) = 0;

        /* These are non-owning. */
        virtual void addFd(int fd, std::function<void()>&& callback) = 0;
        virtual void removeFd(int fd)                                = 0;

        /*
            Get the system icon factory object,
            from which you can lookup icons.
        */
        virtual Hyprutils::Memory::CSharedPointer<ISystemIconFactory> systemIcons() = 0;

        /*
            Add a timer func. This will return a pointer, but the pointer doesn't need
            to be kept.
        */
        virtual Hyprutils::Memory::CAtomicSharedPointer<CTimer> addTimer(const std::chrono::system_clock::duration&                                            timeout,
                                                                         std::function<void(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self, void* data)> cb_, void* data,
                                                                         bool force = false) = 0;

        /*
            Add an idle func. This fn will be executed as soon as possible, but
            after every pending event
        */
        virtual void addIdle(const std::function<void()>& fn) = 0;

        /*
            Enter the loop.
        */
        virtual void                                        enterLoop() = 0;

        virtual Hyprutils::Memory::CSharedPointer<CPalette> getPalette() = 0;

        /*
            Get currently registered outputs.
            Make sure you register the `removed` event to get rid of your reference once the output is removed.
        */
        virtual std::vector<Hyprutils::Memory::CSharedPointer<IOutput>> getOutputs() = 0;

        /*
            Create and lock the graphical session.
            It is required to call this before HT_WINDOW_LOCK_SURFACE can be used.
        */
        virtual std::expected<Hyprutils::Memory::CSharedPointer<ISessionLockState>, eSessionLockError> aquireSessionLock() = 0;

        /*
            Add a log connection for a HU CLogger.
        */
        virtual void setLogConnection(Hyprutils::CLI::CLoggerConnection&& conn) = 0;

        struct {
            /*
                Get notified when a new output was added.
            */
            Hyprutils::Signal::CSignalT<Hyprutils::Memory::CSharedPointer<IOutput>> outputAdded;
        } m_events;

      protected:
        IBackend();
    };
};
