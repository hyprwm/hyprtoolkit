#include <algorithm>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include "InternalBackend.hpp"
#include "AnimationManager.hpp"

#include "./platforms/WaylandPlatform.hpp"
#include "../renderer/gl/OpenGL.hpp"
#include "../output/WaylandOutput.hpp"
#include "../window/WaylandLayer.hpp"
#include "../window/WaylandLockSurface.hpp"
#include "../window/WaylandWindow.hpp"
#include "../Macros.hpp"
#include "../element/Element.hpp"
#include "../palette/ConfigManager.hpp"
#include "../system/Icons.hpp"
#include "../sessionLock/WaylandSessionLock.hpp"

#include <sys/wait.h>
#include <sys/poll.h>

#include <print>
#include <unistd.h>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;

#define SP CSharedPointer
#define WP CWeakPointer

CBackend::CBackend() {
    pipe(m_sLoopState.exitfd);
    pipe(m_sLoopState.wakeupfd);

    Aquamarine::SBackendOptions options{};
    g_logger->m_aqLoggerConnection = makeShared<Hyprutils::CLI::CLoggerConnection>(g_logger->m_logger);
    g_logger->m_aqLoggerConnection->setLogLevel(Hyprutils::CLI::LOG_WARN); // don't print debug logs, unless AQ_TRACE is set, then aq will set it
    g_logger->m_aqLoggerConnection->setName("aquamarine");
    options.logConnection = g_logger->m_aqLoggerConnection;

    std::vector<Aquamarine::SBackendImplementationOptions> implementations;
    Aquamarine::SBackendImplementationOptions              option;
    option.backendType        = Aquamarine::eBackendType::AQ_BACKEND_NULL;
    option.backendRequestMode = Aquamarine::eBackendRequestMode::AQ_BACKEND_REQUEST_MANDATORY;
    implementations.emplace_back(option);

    m_aqBackend             = Aquamarine::CBackend::create(implementations, options);
    g_asyncResourceGatherer = makeShared<CAsyncResourceGatherer>();
    g_animationManager      = makeShared<CHTAnimationManager>();
}

CBackend::~CBackend() {
    destroy();

    close(m_sLoopState.exitfd[0]);
    close(m_sLoopState.exitfd[1]);
    close(m_sLoopState.wakeupfd[0]);
    close(m_sLoopState.wakeupfd[1]);
}

IBackend::SBackendCreationData::SBackendCreationData() = default;

SP<IBackend> IBackend::createWithData(const IBackend::SBackendCreationData& data) {
    g_logger->m_loggerConnection = data.pLogConnection;
    g_logger->updateLogLevel();
    return IBackend::create();
}

SP<IBackend> IBackend::create() {
    if (g_backend)
        return nullptr;
    g_backend = SP<CBackend>(new CBackend());
    g_config  = makeShared<CConfigManager>();
    g_config->parse();
    g_palette     = CPalette::palette();
    g_iconFactory = SP<CSystemIconFactory>(new CSystemIconFactory());
    if (!g_backend->m_aqBackend || !g_backend->m_aqBackend->start()) {
        g_logger->log(HT_LOG_ERROR, "couldn't start aq backend");
        return nullptr;
    }
    g_waylandPlatform = makeUnique<CWaylandPlatform>();
    if (!g_waylandPlatform->attempt()) {
        g_waylandPlatform = nullptr;
        return nullptr;
    }
    g_openGL   = makeShared<COpenGLRenderer>(g_waylandPlatform->m_drmState.fd);
    g_renderer = g_openGL;

    return g_backend;
};

void CBackend::destroy() {
    terminate();
}

void CBackend::setLogFn(LogFn&& fn) {
    g_logger->m_logFn = std::move(fn);
}

SP<CPalette> CBackend::getPalette() {
    return g_palette;
}

std::vector<SP<IOutput>> CBackend::getOutputs() {
    if (!g_waylandPlatform)
        return {};

    return std::vector<SP<IOutput>>(g_waylandPlatform->m_outputs.begin(), g_waylandPlatform->m_outputs.end());
}

std::expected<SP<ISessionLockState>, eSessionLockError> CBackend::aquireSessionLock() {
    if (!g_waylandPlatform)
        return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);

    auto lockState = g_waylandPlatform->aquireSessionLock();
    if (!lockState || lockState->m_denied)
        return std::unexpected(LOCK_ERROR_DENIED);

    return lockState;
}

SP<IWindow> CBackend::openWindow(const SWindowCreationData& data) {
    if (!g_waylandPlatform)
        return nullptr;

    if (data.type == HT_WINDOW_LAYER) {
        if (!g_waylandPlatform->m_waylandState.layerShell)
            return nullptr;

        auto w                         = makeShared<CWaylandLayer>(data);
        w->m_self                      = w;
        w->m_rootElement->impl->window = w;
        g_waylandPlatform->m_layers.emplace_back(w);
        return w;
    } else if (data.type == HT_WINDOW_LOCK_SURFACE) {
        if (!g_waylandPlatform->m_waylandState.sessionLock) {
            g_logger->log(HT_LOG_ERROR, "No session lock manager. Does your compositor support it?");
            return nullptr;
        }

        if (!g_waylandPlatform->m_sessionLockState || g_waylandPlatform->m_sessionLockState->m_denied || g_waylandPlatform->m_sessionLockState->m_sessionUnlocked)
            return nullptr;

        auto w                         = makeShared<CWaylandLockSurface>(data);
        w->m_self                      = w;
        w->m_rootElement->impl->window = w;
        g_waylandPlatform->m_sessionLockState->m_lockSurfaces.emplace_back(w);
        return w;
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
    if (m_terminate)
        return;

    m_terminate = true;

    if (m_sLoopState.eventLoopMutex.try_lock()) {
        m_sLoopState.event = true;
        m_sLoopState.loopCV.notify_all();
        m_sLoopState.eventLoopMutex.unlock();
    }
}

SP<ISystemIconFactory> CBackend::systemIcons() {
    return g_iconFactory;
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

        for (const auto& p : w->m_popups) {
            if (!p)
                continue;

            reloadRecurse(p->m_rootElement);
        }
    }

    for (const auto& w : g_waylandPlatform->m_layers) {
        if (!w)
            continue;

        reloadRecurse(w->m_rootElement);

        for (const auto& p : w->m_popups) {
            if (!p)
                continue;

            reloadRecurse(p->m_rootElement);
        }
    }
}

void CBackend::addFd(int fd, std::function<void()>&& callback) {
    m_sLoopState.userFds.emplace_back(SFDListener{
        .fd       = fd,
        .callback = std::move(callback),
    });

    rebuildPollfds();
}

void CBackend::removeFd(int fd) {
    std::erase_if(m_sLoopState.userFds, [fd](const auto& e) { return e.fd == fd; });
    rebuildPollfds();
}

void CBackend::doOnReadable(Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& fn) {
    int fdInt = fd.get();
    m_sLoopState.userFds.emplace_back(SFDListener{
        .fdOwned      = std::move(fd),
        .fd           = fdInt,
        .callback     = std::move(fn),
        .removeOnFire = true,
    });

    rebuildPollfds();
}

constexpr size_t INTERNAL_FDS = 4;

void             CBackend::rebuildPollfds(bool wakeup) {
    m_pollfds.resize(INTERNAL_FDS + m_sLoopState.userFds.size());

    m_pollfds[0] = {
                    .fd     = wl_display_get_fd(g_waylandPlatform->m_waylandState.display),
                    .events = POLLIN,
    };
    m_pollfds[1] = {
                    .fd     = m_sLoopState.exitfd[0],
                    .events = POLLIN,
    };
    m_pollfds[2] = {
                    .fd     = g_config->m_inotifyFd.get(),
                    .events = POLLIN,
    };
    m_pollfds[3] = {
                    .fd     = m_sLoopState.wakeupfd[0],
                    .events = POLLIN,
    };

    int i = INTERNAL_FDS;

    for (const auto& uf : m_sLoopState.userFds) {
        m_pollfds[i++] = {
                        .fd     = uf.fd,
                        .events = POLLIN,
        };
    }

    if (wakeup)
        write(m_sLoopState.wakeupfd[1], "hello", 5);
}

void CBackend::enterLoop() {

    rebuildPollfds();

    std::thread pollThr([this]() {
        while (!m_terminate) {
            bool preparedToRead = wl_display_prepare_read(g_waylandPlatform->m_waylandState.display) == 0;

            int  events = 0;
            if (preparedToRead) {
                events = poll(m_pollfds.data(), m_pollfds.size(), 5000);

                if (m_terminate)
                    return;

                if (events < 0) {
                    RASSERT(errno == EINTR, "[core] Polling fds failed with {}", errno);
                    wl_display_cancel_read(g_waylandPlatform->m_waylandState.display);
                    continue;
                }

                for (size_t i = 0; i < 1; ++i) {
                    RASSERT(!(m_pollfds[i].revents & POLLHUP), "[core] Disconnected from pollfd id {}", i);
                }

                wl_display_read_events(g_waylandPlatform->m_waylandState.display);
                m_sLoopState.wlDispatched = false;
            }

            m_needsConfigReload = m_pollfds[2].revents & POLLIN;

            if (m_pollfds[3].revents & POLLIN) {
                // clear the wakeup fd
                static std::array<char, 1024> buf;
                read(m_pollfds[3].fd, buf.data(), 1023);
                m_pollfds[3].revents &= ~POLLIN;
            }

            for (size_t i = INTERNAL_FDS; i < m_pollfds.size(); ++i) {
                if (m_pollfds[i].revents & POLLIN)
                    m_sLoopState.userFds[i - INTERNAL_FDS].needsDispatch = true;
            }

            if (events > 0 || !preparedToRead || m_needsConfigReload || (m_pollfds[3].revents & POLLIN) /* wakeup fd */) {
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

        passed.clear();

        // do idles
        m_sLoopState.idlesMutex.lock();
        auto idlesCpy = m_idles;
        m_idles.clear();
        m_sLoopState.idlesMutex.unlock();

        while (!idlesCpy.empty()) {
            for (const auto& i : idlesCpy) {
                (*i)();
            }

            m_sLoopState.idlesMutex.lock();
            idlesCpy = m_idles;
            m_idles.clear();
            m_sLoopState.idlesMutex.unlock();
        }

        if (m_needsConfigReload) {
            m_needsConfigReload = false;
            g_config->onInotifyEvent();
            reloadTheme();
        }

        // do user fds
        std::vector<int> expiredFds;

        for (auto& uf : m_sLoopState.userFds) {
            if (!uf.needsDispatch)
                continue;

            uf.needsDispatch = false;
            uf.callback();

            if (uf.removeOnFire)
                expiredFds.emplace_back(uf.fd);
        }

        if (!expiredFds.empty()) {
            std::erase_if(m_sLoopState.userFds, [&expiredFds](const auto& e) { return std::ranges::contains(expiredFds, e.fd); });
            rebuildPollfds(false);
        }
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

    write(m_sLoopState.exitfd[1], "hello", 5);

    if (timersThr.joinable())
        timersThr.join();

    if (idleThr.joinable())
        idleThr.join();

    if (pollThr.joinable())
        pollThr.join();
}
