#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/BackendServices.hpp>
#include <hyprutils/eventLoop/EventLoop.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <hyprutils/cli/Logger.hpp>
#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>

#include <atomic>
#include <mutex>
#include <thread>

#include "../helpers/Env.hpp"
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {

    class CPalette;
    class CConfigManager;
    class CSystemIconFactory;

    class CBackend : public IBackend, public IEventLoop {
      public:
        CBackend();
        virtual ~CBackend();

        virtual void                     destroy();
        virtual void                     setLogFn(LogFn&& fn);
        virtual void                     addFd(int fd, std::function<void()>&& callback);
        virtual void                     removeFd(int fd);
        virtual SP<ISystemIconFactory>   systemIcons();
        virtual ASP<CTimer>              addTimer(const TimerDuration& timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data, bool force = false);
        virtual void                     addIdle(const std::function<void()>& fn);
        virtual void                     cancelPending();
        virtual void                     enterLoop();
        virtual std::vector<SP<IOutput>> getOutputs();
        virtual SP<CPalette>             getPalette();
        virtual std::expected<SP<ISessionLockState>, eSessionLockError> aquireSessionLock();

        // ======================= Internal fns ======================= //

        void terminate();
        void reloadTheme();
        void updateTimer(CTimer* timer, const std::chrono::steady_clock::time_point& expires);
        void cancelTimer(CTimer* timer);

        // schedule function to when fd is readable (WL_EVENT_READABLE / POLLIN),
        // takes ownership of fd
        void        doOnReadable(Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& fn);

        SP<IWindow> openWindow(const SWindowCreationData& data);

        //

        Hyprutils::Memory::CSharedPointer<Aquamarine::CBackend> m_aqBackend;

        std::atomic<bool>                                       m_terminate = false;
        std::atomic<bool>                                       m_cleaned   = false;
        std::mutex                                              m_loopStateMutex;
        bool                                                    m_loopRunning = false;
        std::thread::id                                         m_loopThread;

        struct SFDListener {
            int                                                                fd = -1;
            Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::IFDSource> source;
        };

        struct STimer {
            Hyprutils::Memory::CAtomicSharedPointer<CTimer>                 timer;
            Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::ITimer> loopTimer;
        };

        bool initializeEventLoop();
        void dispatchWayland(Hyprutils::EventLoop::IFDSource& source, Hyprutils::EventLoop::FdEventMask events);
        void flushWayland(Hyprutils::EventLoop::IFDSource& source);
        void cleanup();
        void registerTimer(const Hyprutils::Memory::CAtomicSharedPointer<CTimer>& timer, const std::chrono::steady_clock::time_point& expires);
        void removeTimer(CTimer* timer);

        Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::IEventLoop>          m_eventLoop;
        Hyprutils::Memory::CAtomicSharedPointer<Hyprutils::EventLoop::ILoopExecutor> m_eventLoopExecutor;
        Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::IFDSource>           m_waylandSource;
        Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::IFDSource>           m_configSource;
        Hyprutils::Memory::CSharedPointer<Hyprutils::EventLoop::IPostDispatchHook>   m_waylandPostDispatch;
        bool                                                                         m_waylandWantsWrite = false;
        std::atomic<uint64_t>                                                        m_pendingGeneration = 0;

        std::vector<SFDListener>                                                     m_userFds;
        std::vector<STimer>                                                          m_timers;
    };
}
