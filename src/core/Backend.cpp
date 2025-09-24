#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include "InternalBackend.hpp"
#include "AnimationManager.hpp"

#include "./platforms/WaylandPlatform.hpp"
#include "../renderer/gl/OpenGL.hpp"
#include "../window/WaylandWindow.hpp"
#include "../Macros.hpp"
#include "../helpers/Timer.hpp"
#include "../element/Element.hpp"
#include "../palette/ConfigManager.hpp"

#include <sys/wait.h>
#include <sys/poll.h>

#include <print>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

static void aqLog(Aquamarine::eBackendLogLevel level, std::string msg) {
    if (g_logger)
        g_logger->log(HT_LOG_DEBUG, "[AQ] {}", msg);
    else
        std::println("{}", msg);
}

CBackend::CBackend() {
    Aquamarine::SBackendOptions options{};
    options.logFunction = ::aqLog;

    std::vector<Aquamarine::SBackendImplementationOptions> implementations;
    Aquamarine::SBackendImplementationOptions              option;
    option.backendType        = Aquamarine::eBackendType::AQ_BACKEND_NULL;
    option.backendRequestMode = Aquamarine::eBackendRequestMode::AQ_BACKEND_REQUEST_MANDATORY;
    implementations.emplace_back(option);

    m_aqBackend             = Aquamarine::CBackend::create(implementations, options);
    g_asyncResourceGatherer = makeShared<CAsyncResourceGatherer>();
    g_animationManager      = makeShared<CHTAnimationManager>();
}

SP<CBackend> CBackend::create() {
    if (g_backend)
        return nullptr;
    g_backend = SP<CBackend>(new CBackend());
    g_logger  = SP<CBackendLogger>(new CBackendLogger());
    g_config  = makeShared<CConfigManager>();
    g_config->parse();
    g_palette = CPalette::palette();
    if (!g_backend->m_aqBackend || !g_backend->m_aqBackend->start()) {
        g_logger->log(HT_LOG_ERROR, "couldn't start aq backend");
        return nullptr;
    }

    return g_backend;
};

void CBackend::destroy() {

    terminate();
}

void CBackend::setLogFn(LogFn&& fn) {
    m_logFn = std::move(fn);
}

SP<CPalette> CBackend::getPalette() {
    return g_palette;
}

SP<IWindow> CBackend::openWindow(const SWindowCreationData& data) {
    if (!g_waylandPlatform) {
        g_waylandPlatform = makeUnique<CWaylandPlatform>();
        if (!g_waylandPlatform->attempt())
            return nullptr;
        g_openGL   = makeShared<COpenGLRenderer>(g_waylandPlatform->m_drmState.fd);
        g_renderer = g_openGL;
    }

    auto w                         = makeShared<CWaylandWindow>(data);
    w->m_self                      = w;
    w->m_rootElement->impl->window = w;
    g_waylandPlatform->m_windows.emplace_back(w);
    return w;
}

ASP<CTimer> CBackend::addTimer(const std::chrono::system_clock::duration& timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data, bool force) {
    std::lock_guard<std::mutex> lg(m_sLoopState.timersMutex);
    const auto                  T = m_timers.emplace_back(makeAtomicShared<CTimer>(timeout, cb_, data, force));
    m_sLoopState.timerEvent       = true;
    m_sLoopState.timerCV.notify_all();
    return T;
}

void CBackend::addIdle(const std::function<void()>& fn) {
    std::lock_guard<std::mutex> lg(m_sLoopState.idlesMutex);
    m_idles.emplace_back(makeAtomicShared<std::function<void()>>(fn));
    m_sLoopState.idleEvent = true;
    m_sLoopState.idleCV.notify_all();
}

void CBackend::terminate() {
    m_terminate = true;

    if (m_sLoopState.eventLoopMutex.try_lock()) {
        m_sLoopState.event = true;
        m_sLoopState.loopCV.notify_all();
        m_sLoopState.eventLoopMutex.unlock();
    }
}

static void reloadRecurse(SP<IElement> el) {
    for (const auto& e : el->impl->children) {
        if (!e)
            continue;

        e->recheckColor();

        reloadRecurse(e);
    }
}

void CBackend::reloadTheme() {
    if (g_palette->m_isConfig)
        g_palette = CPalette::palette();

    for (const auto& w : g_waylandPlatform->m_windows) {
        if (!w)
            continue;

        reloadRecurse(w->m_rootElement);
    }
}

void CBackend::enterLoop() {
    int exitfd[2];
    pipe(exitfd);

    pollfd pollfds[3];
    pollfds[0] = {
        .fd     = wl_display_get_fd(g_waylandPlatform->m_waylandState.display),
        .events = POLLIN,
    };
    pollfds[1] = {
        .fd     = exitfd[0],
        .events = POLLIN,
    };
    pollfds[2] = {
        .fd     = g_config->m_inotifyFd.get(),
        .events = POLLIN,
    };

    std::thread pollThr([this, &pollfds]() {
        while (!m_terminate) {
            bool preparedToRead = wl_display_prepare_read(g_waylandPlatform->m_waylandState.display) == 0;

            int  events = 0;
            if (preparedToRead) {
                events = poll(pollfds, 3, 5000);

                if (m_terminate)
                    return;

                if (events < 0) {
                    RASSERT(errno == EINTR, "[core] Polling fds failed with {}", errno);
                    wl_display_cancel_read(g_waylandPlatform->m_waylandState.display);
                    continue;
                }

                for (size_t i = 0; i < 1; ++i) {
                    RASSERT(!(pollfds[i].revents & POLLHUP), "[core] Disconnected from pollfd id {}", i);
                }

                wl_display_read_events(g_waylandPlatform->m_waylandState.display);
                m_sLoopState.wlDispatched = false;
            }

            m_needsConfigReload = pollfds[2].revents & POLLIN;

            if (events > 0 || !preparedToRead || m_needsConfigReload) {
                std::unique_lock lk(m_sLoopState.eventLoopMutex);
                m_sLoopState.event = true;
                m_sLoopState.loopCV.notify_all();

                m_sLoopState.wlDispatchCV.wait_for(lk, std::chrono::milliseconds(100), [this] { return m_sLoopState.wlDispatched; });
            }
        }
    });

    std::thread timersThr([this]() {
        while (!m_terminate) {
            // calc nearest thing
            m_sLoopState.timersMutex.lock();

            float least = 10000;
            for (auto& t : m_timers) {
                const auto TIME = std::clamp(t->leftMs(), 1.f, INFINITY);
                least           = std::min(TIME, least);
            }

            m_sLoopState.timersMutex.unlock();

            std::unique_lock lk(m_sLoopState.timerRequestMutex);
            m_sLoopState.timerCV.wait_for(lk, std::chrono::milliseconds((int)least + 1), [this] { return m_sLoopState.timerEvent; });
            m_sLoopState.timerEvent = false;

            // notify main
            std::lock_guard<std::mutex> lg2(m_sLoopState.eventLoopMutex);
            m_sLoopState.event = true;
            m_sLoopState.loopCV.notify_all();
        }
    });

    std::thread idleThr([this]() {
        while (!m_terminate) {
            std::unique_lock lk(m_sLoopState.timerRequestMutex);
            m_sLoopState.idleCV.wait(lk, [this] { return m_sLoopState.idleEvent; });
            m_sLoopState.idleEvent = false;

            // notify main
            std::lock_guard<std::mutex> lg2(m_sLoopState.eventLoopMutex);
            m_sLoopState.event = true;
            m_sLoopState.loopCV.notify_all();
        }
    });

    m_sLoopState.event = true; // let it process once

    while (!m_terminate) {
        std::unique_lock lk(m_sLoopState.eventRequestMutex);
        if (!m_sLoopState.event)
            m_sLoopState.loopCV.wait_for(lk, std::chrono::milliseconds(5000), [this] { return m_sLoopState.event; });

        if (m_terminate)
            break;

        std::lock_guard<std::mutex> lg(m_sLoopState.eventLoopMutex);

        m_sLoopState.event = false;

        wl_display_dispatch_pending(g_waylandPlatform->m_waylandState.display);
        wl_display_flush(g_waylandPlatform->m_waylandState.display);

        m_sLoopState.wlDispatched = true;
        m_sLoopState.wlDispatchCV.notify_all();

        // do timers
        m_sLoopState.timersMutex.lock();
        auto timerscpy = m_timers;
        m_sLoopState.timersMutex.unlock();

        std::vector<ASP<CTimer>> passed;

        for (auto& t : timerscpy) {
            if (t->passed() && !t->cancelled()) {
                t->call(t);
                passed.push_back(t);
            }

            if (t->cancelled())
                passed.push_back(t);
        }

        m_sLoopState.timersMutex.lock();
        std::erase_if(m_timers, [passed](const auto& timer) { return std::find(passed.begin(), passed.end(), timer) != passed.end(); });
        m_sLoopState.timersMutex.unlock();

        // do idles
        m_sLoopState.idlesMutex.lock();
        auto idlesCpy = m_idles;
        m_idles.clear();
        m_sLoopState.idlesMutex.unlock();

        for (const auto& i : idlesCpy) {
            (*i)();
        }

        if (m_needsConfigReload) {
            m_needsConfigReload = false;
            g_config->onInotifyEvent();
            reloadTheme();
        }

        passed.clear();
    }

    g_renderer.reset();
    g_openGL.reset();

    g_waylandPlatform.reset();

    g_asyncResourceGatherer.reset();
    g_animationManager.reset();

    g_palette.reset();
    g_backend.reset();
    g_logger.reset();

    m_sLoopState.idleEvent = true;
    m_sLoopState.idleCV.notify_all();

    m_sLoopState.timerEvent = true;
    m_sLoopState.timerCV.notify_all();

    write(exitfd[1], "hello", 5);

    if (timersThr.joinable())
        timersThr.join();

    if (idleThr.joinable())
        idleThr.join();

    if (pollThr.joinable())
        pollThr.join();
}
