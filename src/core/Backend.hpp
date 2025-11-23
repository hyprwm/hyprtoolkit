#pragma once

#include <hyprtoolkit/core/Backend.hpp>
#include <hyprutils/os/FileDescriptor.hpp>
#include <hyprutils/cli/Logger.hpp>
#include <hyprgraphics/resource/AsyncResourceGatherer.hpp>

#include "../helpers/Env.hpp"
#include "../helpers/Memory.hpp"

namespace Hyprtoolkit {

    class CPalette;
    class CConfigManager;
    class CSystemIconFactory;

    class CBackend : public IBackend {
      public:
        CBackend();
        virtual ~CBackend();

        virtual void                   destroy();
        virtual void                   setLogFn(LogFn&& fn);
        virtual void                   setLogConnection(Hyprutils::CLI::CLoggerConnection&& conn);
        virtual void                   addFd(int fd, std::function<void()>&& callback);
        virtual void                   removeFd(int fd);
        virtual SP<ISystemIconFactory> systemIcons();
        virtual ASP<CTimer> addTimer(const std::chrono::system_clock::duration& timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data, bool force = false);
        virtual void        addIdle(const std::function<void()>& fn);
        virtual void        enterLoop();
        virtual std::vector<SP<IOutput>>                                getOutputs();
        virtual SP<CPalette>                                            getPalette();
        virtual std::expected<SP<ISessionLockState>, eSessionLockError> aquireSessionLock();

        // ======================= Internal fns ======================= //

        void terminate();
        void reloadTheme();
        void rebuildPollfds(bool wakeup = true);

        // schedule function to when fd is readable (WL_EVENT_READABLE / POLLIN),
        // takes ownership of fd
        void        doOnReadable(Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& fn);

        SP<IWindow> openWindow(const SWindowCreationData& data);

        //

        std::vector<pollfd>                                     m_pollfds;

        Hyprutils::Memory::CSharedPointer<Aquamarine::CBackend> m_aqBackend;

        bool                                                    m_terminate         = false;
        bool                                                    m_needsConfigReload = false;

        struct SFDListener {
            Hyprutils::OS::CFileDescriptor fdOwned;
            int                            fd = 0;
            std::function<void()>          callback;
            bool                           needsDispatch = false;
            bool                           removeOnFire  = false;
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
            int                      wakeupfd[2];

            std::vector<SFDListener> userFds;
        } m_sLoopState;

        std::vector<Hyprutils::Memory::CAtomicSharedPointer<CTimer>>                m_timers;
        std::vector<Hyprutils::Memory::CAtomicSharedPointer<std::function<void()>>> m_idles;
    };
}
