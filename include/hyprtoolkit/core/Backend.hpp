#pragma once

#include <hyprutils/memory/SharedPtr.hpp>
#include <hyprutils/memory/Atomic.hpp>
#include <aquamarine/backend/Null.hpp>
#include <aquamarine/backend/Backend.hpp>
#include <functional>
#include <string>

#include "LogTypes.hpp"
#include "../palette/Palette.hpp"
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

      private:
        CBackend();

        void                                                    terminate();
        void                                                    reloadTheme();

        Hyprutils::Memory::CSharedPointer<Aquamarine::CBackend> m_aqBackend;

        LogFn                                                   m_logFn;

        bool                                                    m_terminate         = false;
        bool                                                    m_needsConfigReload = false;

        struct {
            std::mutex              timersMutex;
            std::mutex              idlesMutex;
            std::mutex              eventRequestMutex;
            std::mutex              eventLoopMutex;
            std::condition_variable loopCV;
            bool                    event = false;

            std::condition_variable wlDispatchCV;
            bool                    wlDispatched = false;

            std::condition_variable timerCV;
            std::mutex              timerRequestMutex;
            bool                    timerEvent = false;

            std::condition_variable idleCV;
            std::mutex              idleRequestMutex;
            bool                    idleEvent = false;
        } m_sLoopState;

        std::vector<Hyprutils::Memory::CAtomicSharedPointer<CTimer>>                m_timers;
        std::vector<Hyprutils::Memory::CAtomicSharedPointer<std::function<void()>>> m_idles;

        friend class CBackendLogger;
        friend class CWaylandPlatform;
        friend class CWaylandWindow;
        friend class CGLTexture;
        friend class CTextElement;
        friend class CImageElement;
        friend class CWaylandPopup;
    };
};
