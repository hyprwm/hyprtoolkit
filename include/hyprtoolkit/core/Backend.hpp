#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <aquamarine/backend/Null.hpp>
#include <aquamarine/backend/Backend.hpp>
#include <functional>
#include <string>

#include "LogTypes.hpp"
#include "../window/WindowTypes.hpp"

namespace Hyprtoolkit {
    class IWindow;
    class CTimer;

    class CBackend {
      public:
        using LogFn = std::function<void(eLogLevel, const std::string&)>;

        /*
            Create a backend.
            There can only be one backend per process: In case of another create(),
            it will fail.
        */
        static Hyprutils::Memory::CSharedPointer<CBackend> create();

        /*
            Destroy the backend.
            Backend will be destroyed once:
             - All refs YOU hold are dead
             - You call this fn
        */
        void destroy();

        ~CBackend() = default;

        void setLogFn(LogFn&& fn);

        /*
            Open a window. This opens a toplevel window. Each window has a root
            element from which you can start building your scene.
        */
        Hyprutils::Memory::CSharedPointer<IWindow> openWindow(const SWindowCreationData& data);

        /*
            Enter the loop.
        */
        void enterLoop();

      private:
        CBackend();

        Hyprutils::Memory::CSharedPointer<Aquamarine::CBackend> m_aqBackend;

        LogFn                                                   m_logFn;

        bool                                                    m_terminate = false;

        struct {
            std::mutex              timersMutex;
            std::mutex              eventRequestMutex;
            std::mutex              eventLoopMutex;
            std::condition_variable loopCV;
            bool                    event = false;

            std::condition_variable wlDispatchCV;
            bool                    wlDispatched = false;

            std::condition_variable timerCV;
            std::mutex              timerRequestMutex;
            bool                    timerEvent = false;
        } m_sLoopState;

        std::vector<Hyprutils::Memory::CAtomicSharedPointer<CTimer>> m_timers;

        friend class CBackendLogger;
        friend class CWaylandPlatform;
        friend class CWaylandWindow;
    };
};
