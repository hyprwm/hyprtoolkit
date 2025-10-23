#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <aquamarine/backend/Null.hpp>
#include <aquamarine/backend/Backend.hpp>
#include <functional>
#include <string>
#include <sys/poll.h>

#include "LogTypes.hpp"
#include "../palette/Palette.hpp"

#include "CoreMacros.hpp"

namespace Hyprtoolkit {
    class IWindow;
    class IOutput;
    class CTimer;
    class ISystemIconFactory;
    struct SWindowCreationData;

    class CBackend {
      public:
        ~CBackend();

        using LogFn = std::function<void(eLogLevel, const std::string&)>;

        /*
            Create a backend.
            There can only be one backend per process: In case of another create(),
            it will fail.
        */
        static Hyprutils::Memory::CSharedPointer<CBackend> create();

        /*
            Attempt to initialize the platform.
            Optional. Can be used in case the outputAdded/Removed events are used to create new windows.
            In such a szenario the platform must be initialized before opening a window.
        */
        static bool attempt();

        /*
            Destroy the backend.
            Backend will be destroyed once:
             - All refs YOU hold are dead
             - You call this fn
        */
        void destroy();

        void setLogFn(LogFn&& fn);

        /* These are non-owning. */
        void addFd(int fd, std::function<void()>&& callback);
        void removeFd(int fd);

        /*
            Get the system icon factory object,
            from which you can lookup icons.
        */
        Hyprutils::Memory::CSharedPointer<ISystemIconFactory> systemIcons();

        /*
            Add a timer func. This will return a pointer, but the pointer doesn't need
            to be kept.
        */
        Hyprutils::Memory::CAtomicSharedPointer<CTimer> addTimer(const std::chrono::system_clock::duration&                                            timeout,
                                                                 std::function<void(Hyprutils::Memory::CAtomicSharedPointer<CTimer> self, void* data)> cb_, void* data,
                                                                 bool force = false);

        /*
            Add an idle func. This fn will be executed as soon as possible, but
            after every pending event
        */
        void addIdle(const std::function<void()>& fn);

        /*
            Enter the loop.
        */
        void                                        enterLoop();

        Hyprutils::Memory::CSharedPointer<CPalette> getPalette();

        void                                        unlockSession();

        /*
            Get currently registered outputs.
            Make sure you register the `removed` event to get rid of your reference once the output is removed.
        */
        std::vector<Hyprutils::Memory::CSharedPointer<IOutput>> getOutputs();

        struct {
            /*
                Get notified when a new output was added.
            */
            Hyprutils::Signal::CSignalT<Hyprutils::Memory::CSharedPointer<IOutput>> outputAdded;
        } m_events;

        HT_HIDDEN : CBackend();

        void                                                    terminate();
        void                                                    reloadTheme();
        void                                                    rebuildPollfds();

        Hyprutils::Memory::CSharedPointer<IWindow>              openWindow(const SWindowCreationData& data);

        std::vector<pollfd>                                     m_pollfds;

        Hyprutils::Memory::CSharedPointer<Aquamarine::CBackend> m_aqBackend;

        LogFn                                                   m_logFn;

        bool                                                    m_terminate         = false;
        bool                                                    m_needsConfigReload = false;

        struct SFDListener {
            int                   fd = 0;
            std::function<void()> callback;
            bool                  needsDispatch = false;
        };

        struct {
            std::mutex               timersMutex;
            std::mutex               idlesMutex;
            std::mutex               eventRequestMutex;
            std::mutex               eventLoopMutex;
            std::condition_variable  loopCV;
            bool                     event = false;

            std::condition_variable  wlDispatchCV;
            bool                     wlDispatched = false;

            std::condition_variable  timerCV;
            std::mutex               timerRequestMutex;
            bool                     timerEvent = false;

            std::condition_variable  idleCV;
            std::mutex               idleRequestMutex;
            bool                     idleEvent = false;

            int                      exitfd[2];

            std::vector<SFDListener> userFds;
        } m_sLoopState;

        std::vector<Hyprutils::Memory::CAtomicSharedPointer<CTimer>>                m_timers;
        std::vector<Hyprutils::Memory::CAtomicSharedPointer<std::function<void()>>> m_idles;
    };
};
