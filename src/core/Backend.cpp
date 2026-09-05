#include <algorithm>
#include <hyprtoolkit/core/Backend.hpp>
#include <hyprtoolkit/core/Timer.hpp>
#include <hyprtoolkit/palette/Palette.hpp>

#include "InternalBackend.hpp"
#include "BackendContext.hpp"
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

#include <cerrno>
#include <fcntl.h>
#include <system_error>

using namespace Hyprtoolkit;
using namespace Hyprutils::Memory;
using namespace Hyprutils::OS;

#define SP CSharedPointer
#define WP CWeakPointer

class CWaylandClipboard final : public IClipboard {
  public:
    void setText(const std::string& text) override {
        g_waylandPlatform->setClipboard(text);
    }

    std::string getText() override {
        return g_waylandPlatform->readClipboard();
    }
};

class CWaylandTextInput final : public ITextInput {
  public:
    void activate(EmbeddedSurfaceID surface, const Hyprutils::Math::CBox& cursorBox, const std::string& surroundingText, size_t cursor) override {
        if (!g_waylandPlatform->m_waylandState.textInput)
            return;

        if (!g_waylandPlatform->m_waylandState.imState.enabled) {
            g_waylandPlatform->m_waylandState.textInput->sendEnable();
            g_waylandPlatform->m_waylandState.imState.enabled = true;
        }

        g_waylandPlatform->m_waylandState.textInput->sendSetCursorRectangle(cursorBox.x, cursorBox.y, cursorBox.w, cursorBox.h);
        g_waylandPlatform->m_waylandState.textInput->sendCommit();
    }

    void deactivate(EmbeddedSurfaceID surface) override {
        if (!g_waylandPlatform->m_waylandState.textInput || !g_waylandPlatform->m_waylandState.imState.enabled)
            return;

        g_waylandPlatform->m_waylandState.textInput->sendDisable();
        g_waylandPlatform->m_waylandState.imState.enabled = false;
    }
};

class CWaylandCursor final : public ICursor {
  public:
    void setShape(EmbeddedSurfaceID surface, ePointerShape shape) override {
        g_waylandPlatform->setCursor(shape);
    }
};

class CWaylandOutputProvider final : public IOutputProvider {
  public:
    std::vector<SP<IOutput>> outputs() override {
        return std::vector<SP<IOutput>>(g_waylandPlatform->m_outputs.begin(), g_waylandPlatform->m_outputs.end());
    }
};

class CWaylandSessionLockProvider final : public ISessionLockProvider {
  public:
    std::expected<SP<ISessionLockState>, eSessionLockError> acquire() override {
        if (!g_waylandPlatform->m_waylandState.sessionLock)
            return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);

        const auto lockState = g_waylandPlatform->aquireSessionLock();
        if (!lockState || lockState->m_denied)
            return std::unexpected(LOCK_ERROR_DENIED);

        return lockState;
    }
};

CBackend::CBackend() {
    const auto eventLoop = Hyprutils::EventLoop::IEventLoop::create();
    if (!eventLoop) {
        g_logger->log(HT_LOG_ERROR, "couldn't create event loop: {}", eventLoop.error());
        return;
    }

    m_eventLoop         = *eventLoop;
    m_eventLoopExecutor = m_eventLoop->executor();

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
    // ~CBackend may run during static destruction, when other inline globals
    // (g_renderer, g_openGL, g_logger, ...) may already be destroyed. touching
    // them here is UB. cleanup of those globals has to happen earlier, see the
    // atexit handler installed in IBackend::create, so we only release our own
    // members here.
    m_terminate = true;
}

IBackend::SBackendCreationData::SBackendCreationData() = default;

SP<IBackend> IBackend::createWithData(const IBackend::SBackendCreationData& data) {
    g_logger                     = makeShared<CLogger>();
    g_logger->m_loggerConnection = data.pLogConnection;
    g_logger->updateLogLevel();
    return IBackend::create();
}

SP<IBackend> IBackend::create() {
    if (g_backend)
        return nullptr;

    if (!g_logger)
        g_logger = makeShared<CLogger>();

    auto backend     = SP<CBackend>(new CBackend());
    g_backend        = backend;
    g_waylandBackend = backend;
    auto bk          = g_backend;

    if (!backend->m_eventLoop) {
        g_waylandBackend.reset();
        g_backend.reset();
        return nullptr;
    }

    g_backendServices             = makeUnique<SBackendServices>(makeDefaultBackendServices());
    g_backendServices->eventLoop  = dynamicPointerCast<IEventLoop>(backend);
    g_backendServices->openWindow = [weak = WP<CBackend>{backend}](const SWindowCreationData& data) -> SP<IWindow> {
        if (const auto locked = weak.lock())
            return locked->openWindow(data);
        return nullptr;
    };
    g_backendServices->doOnReadable = [weak = WP<CBackend>{backend}](Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& callback) {
        if (const auto locked = weak.lock())
            locked->doOnReadable(std::move(fd), std::move(callback));
    };
    g_config = makeShared<CConfigManager>();
    g_config->parse();
    g_palette     = CPalette::palette();
    g_iconFactory = SP<CSystemIconFactory>(new CSystemIconFactory());
    if (!backend->m_aqBackend || !backend->m_aqBackend->start()) {
        g_logger->log(HT_LOG_ERROR, "couldn't start aq backend");
        g_backendServices.reset();
        g_waylandBackend.reset();
        g_backend.reset();
        return nullptr;
    }
    g_waylandPlatform = makeUnique<CWaylandPlatform>();
    if (!g_waylandPlatform->attempt()) {
        g_waylandPlatform = nullptr;
        g_backendServices.reset();
        g_waylandBackend.reset();
        g_backend.reset();
        return nullptr;
    }
    g_backendServices->clipboard   = makeShared<CWaylandClipboard>();
    g_backendServices->textInput   = makeShared<CWaylandTextInput>();
    g_backendServices->cursor      = makeShared<CWaylandCursor>();
    g_backendServices->outputs     = makeShared<CWaylandOutputProvider>();
    g_backendServices->sessionLock = makeShared<CWaylandSessionLockProvider>();
    g_openGL                       = makeShared<COpenGLRenderer>(g_waylandPlatform->m_drmState.fd);
    g_renderer                     = g_openGL;

    if (!backend->initializeEventLoop()) {
        backend->terminate();
        return nullptr;
    }

    // run cleanup while every inline static SP is still alive. by the time
    // ~CBackend reaches static destruction, sibling globals like g_renderer
    // may already have been freed, so any cleanup that touches them must run
    // here. atexit fires after main returns but before static destructors.
    static const int once = std::atexit([]() {
        if (g_backend)
            g_backend->destroy();
    });
    (void)once;

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
    if (!g_backendServices)
        return {};
    return g_backendServices->outputs->outputs();
}

std::expected<SP<ISessionLockState>, eSessionLockError> CBackend::aquireSessionLock() {
    if (!g_backendServices)
        return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);
    return g_backendServices->sessionLock->acquire();
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

ASP<CTimer> CBackend::addTimer(const TimerDuration& timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data, bool force) {
    const auto       timer   = makeAtomicShared<CTimer>(timeout, std::move(cb_), data, force);
    const auto       expires = std::chrono::steady_clock::now() + timeout;

    std::unique_lock lock(m_loopStateMutex);
    if (m_terminate || !m_eventLoop)
        return {};

    if (m_loopRunning && m_loopThread != std::this_thread::get_id()) {
        const auto executor = m_eventLoopExecutor;
        lock.unlock();
        executor->post([this, timer, expires] {
            std::lock_guard lock(m_loopStateMutex);
            registerTimer(timer, expires);
        });
    } else
        registerTimer(timer, expires);

    return timer;
}

void CBackend::registerTimer(const ASP<CTimer>& timer, const std::chrono::steady_clock::time_point& expires) {
    if (m_terminate || timer->cancelled())
        return;

    const auto loopTimer = m_eventLoop->addTimer(expires - std::chrono::steady_clock::now(), [this, timer](Hyprutils::EventLoop::ITimer& self) {
        if (timer->cancelled()) {
            removeTimer(timer.get());
            return;
        }

        if (!timer->passed()) {
            self.updateTimeout(std::chrono::milliseconds(std::max(1, sc<int>(timer->leftMs()))));
            return;
        }

        timer->call(timer);
        removeTimer(timer.get());
    });

    m_timers.emplace_back(STimer{
        .timer     = timer,
        .loopTimer = loopTimer,
    });
}

void CBackend::addIdle(const std::function<void()>& fn) {
    if (!m_eventLoopExecutor || m_terminate)
        return;

    const auto generation = m_pendingGeneration.load();
    m_eventLoopExecutor->post([this, generation, fn] {
        if (generation == m_pendingGeneration)
            fn();
    });
}

void CBackend::cancelPending() {
    std::lock_guard lock(m_loopStateMutex);

    ++m_pendingGeneration;
    m_timers.clear();
    for (const auto& listener : m_userFds)
        listener.source->remove();
    m_userFds.clear();
}

void CBackend::terminate() {
    if (m_terminate.exchange(true))
        return;

    bool loopRunning = false;
    {
        std::lock_guard lock(m_loopStateMutex);
        loopRunning = m_loopRunning;
    }

    if (loopRunning && m_eventLoopExecutor) {
        const auto executor = m_eventLoopExecutor;
        executor->post([this] {
            if (m_eventLoop)
                m_eventLoop->stop();
        });
        return;
    }

    cleanup();
}

void CBackend::removeTimer(CTimer* timer) {
    std::erase_if(m_timers, [timer](const auto& entry) { return entry.timer.get() == timer; });
}

void CBackend::updateTimer(CTimer* timer, const std::chrono::steady_clock::time_point& expires) {
    std::lock_guard lock(m_loopStateMutex);
    if (m_terminate || (m_loopRunning && m_loopThread != std::this_thread::get_id()))
        return;

    const auto entry = std::ranges::find_if(m_timers, [timer](const auto& candidate) { return candidate.timer.get() == timer; });
    if (entry != m_timers.end())
        entry->loopTimer->updateTimeout(expires - std::chrono::steady_clock::now());
}

void CBackend::cancelTimer(CTimer* timer) {
    std::lock_guard lock(m_loopStateMutex);
    if (m_terminate || (m_loopRunning && m_loopThread != std::this_thread::get_id()))
        return;

    removeTimer(timer);
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
    if (!m_eventLoop || m_terminate)
        return;

    CFileDescriptor duplicatedFD{fcntl(fd, F_DUPFD_CLOEXEC, 0)};
    if (!duplicatedFD.isValid()) {
        g_logger->log(HT_LOG_ERROR, "couldn't duplicate fd {} for event loop: {}", fd, std::system_category().message(errno));
        return;
    }

    auto source = m_eventLoop->addFD(std::move(duplicatedFD), Hyprutils::EventLoop::eEventMask::READABLE,
                                     [callback = std::move(callback)](Hyprutils::EventLoop::IFDSource& self, Hyprutils::EventLoop::FdEventMask events) {
                                         if (events & Hyprutils::EventLoop::eEventMask::READABLE)
                                             callback();

                                         if (events & (Hyprutils::EventLoop::eEventMask::HUP | Hyprutils::EventLoop::eEventMask::ERROR))
                                             (void)self.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
                                     });
    if (!source) {
        g_logger->log(HT_LOG_ERROR, "couldn't add fd {} to event loop: {}", fd, source.error());
        return;
    }

    m_userFds.emplace_back(SFDListener{
        .fd     = fd,
        .source = *source,
    });
}

void CBackend::removeFd(int fd) {
    std::erase_if(m_userFds, [fd](const auto& listener) {
        if (listener.fd != fd)
            return false;

        listener.source->remove();
        return true;
    });
}

void CBackend::doOnReadable(Hyprutils::OS::CFileDescriptor fd, std::function<void()>&& fn) {
    if (!m_eventLoop || m_terminate)
        return;

    const int fdInt  = fd.get();
    auto      source = m_eventLoop->addFD(std::move(fd), Hyprutils::EventLoop::eEventMask::READABLE,
                                          [fn = std::move(fn)](Hyprutils::EventLoop::IFDSource& self, Hyprutils::EventLoop::FdEventMask events) mutable {
                                         self.remove();
                                         if (events & Hyprutils::EventLoop::eEventMask::READABLE)
                                             fn();
                                          });
    if (!source)
        g_logger->log(HT_LOG_ERROR, "couldn't add owned fd {} to event loop: {}", fdInt, source.error());
}

bool CBackend::initializeEventLoop() {
    const int       waylandFD = wl_display_get_fd(g_waylandPlatform->m_waylandState.display);
    CFileDescriptor duplicatedWaylandFD{fcntl(waylandFD, F_DUPFD_CLOEXEC, 0)};
    if (!duplicatedWaylandFD.isValid()) {
        g_logger->log(HT_LOG_ERROR, "couldn't duplicate Wayland fd for event loop: {}", std::system_category().message(errno));
        return false;
    }

    auto waylandSource = m_eventLoop->addFD(std::move(duplicatedWaylandFD), Hyprutils::EventLoop::eEventMask::READABLE,
                                            [this](Hyprutils::EventLoop::IFDSource& source, Hyprutils::EventLoop::FdEventMask events) { dispatchWayland(source, events); });
    if (!waylandSource) {
        g_logger->log(HT_LOG_ERROR, "couldn't add Wayland fd to event loop: {}", waylandSource.error());
        return false;
    }
    m_waylandSource = *waylandSource;

    if (g_config->m_inotifyFd.isValid()) {
        auto duplicatedConfigFD = g_config->m_inotifyFd.duplicate();
        if (!duplicatedConfigFD.isValid()) {
            g_logger->log(HT_LOG_ERROR, "couldn't duplicate config fd for event loop: {}", std::system_category().message(errno));
            return false;
        }

        auto configSource = m_eventLoop->addFD(std::move(duplicatedConfigFD), Hyprutils::EventLoop::eEventMask::READABLE,
                                               [this](Hyprutils::EventLoop::IFDSource& source, Hyprutils::EventLoop::FdEventMask events) {
                                                   if (events & Hyprutils::EventLoop::eEventMask::READABLE) {
                                                       g_config->onInotifyEvent();
                                                       reloadTheme();
                                                   }

                                                   if (events & (Hyprutils::EventLoop::eEventMask::HUP | Hyprutils::EventLoop::eEventMask::ERROR)) {
                                                       g_logger->log(HT_LOG_ERROR, "config event fd disconnected");
                                                       (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
                                                   }
                                               });
        if (!configSource) {
            g_logger->log(HT_LOG_ERROR, "couldn't add config fd to event loop: {}", configSource.error());
            return false;
        }
        m_configSource = *configSource;
    }

    m_waylandPostDispatch = m_eventLoop->addPostDispatch([this] {
        if (m_terminate)
            return;

        if (wl_display_dispatch_pending(g_waylandPlatform->m_waylandState.display) < 0) {
            g_logger->log(HT_LOG_ERROR, "failed to dispatch pending Wayland events: {}", wl_display_get_error(g_waylandPlatform->m_waylandState.display));
            (void)m_waylandSource->setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
            terminate();
            return;
        }

        if (!m_terminate)
            flushWayland(*m_waylandSource);
    });

    return true;
}

void CBackend::dispatchWayland(Hyprutils::EventLoop::IFDSource& source, Hyprutils::EventLoop::FdEventMask events) {
    if (m_terminate)
        return;

    const auto DISPLAY = g_waylandPlatform->m_waylandState.display;

    if (events & Hyprutils::EventLoop::eEventMask::READABLE) {
        while (wl_display_prepare_read(DISPLAY) != 0) {
            if (errno != EAGAIN || wl_display_dispatch_pending(DISPLAY) < 0) {
                g_logger->log(HT_LOG_ERROR, "failed to prepare Wayland read: {}", wl_display_get_error(DISPLAY));
                (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
                terminate();
                return;
            }

            if (m_terminate)
                return;
        }

        if (wl_display_read_events(DISPLAY) < 0) {
            g_logger->log(HT_LOG_ERROR, "failed to read Wayland events: {}", wl_display_get_error(DISPLAY));
            (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
            terminate();
            return;
        }

        if (wl_display_dispatch_pending(DISPLAY) < 0) {
            g_logger->log(HT_LOG_ERROR, "failed to dispatch Wayland events: {}", wl_display_get_error(DISPLAY));
            (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
            terminate();
            return;
        }

        if (!m_terminate)
            flushWayland(source);
    }

    if (m_terminate)
        return;

    if (events & (Hyprutils::EventLoop::eEventMask::HUP | Hyprutils::EventLoop::eEventMask::ERROR)) {
        g_logger->log(HT_LOG_ERROR, "Wayland connection closed: {}", wl_display_get_error(DISPLAY));
        (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
        terminate();
        return;
    }

    if (events & Hyprutils::EventLoop::eEventMask::WRITABLE)
        flushWayland(source);
}

void CBackend::flushWayland(Hyprutils::EventLoop::IFDSource& source) {
    int result = 0;
    do {
        result = wl_display_flush(g_waylandPlatform->m_waylandState.display);
    } while (result < 0 && errno == EINTR);

    bool wantsWrite = false;
    if (result < 0) {
        if (errno != EAGAIN) {
            g_logger->log(HT_LOG_ERROR, "failed to flush Wayland connection: {}", wl_display_get_error(g_waylandPlatform->m_waylandState.display));
            (void)source.setMask(Hyprutils::EventLoop::eEventMask::EMPTY);
            terminate();
            return;
        }

        wantsWrite = true;
    }

    if (m_waylandWantsWrite == wantsWrite)
        return;

    const auto mask = wantsWrite ? Hyprutils::EventLoop::eEventMask::READABLE | Hyprutils::EventLoop::eEventMask::WRITABLE :
                                   Hyprutils::EventLoop::FdEventMask{Hyprutils::EventLoop::eEventMask::READABLE};
    if (const auto updated = source.setMask(mask); !updated) {
        g_logger->log(HT_LOG_ERROR, "failed to update Wayland event mask: {}", updated.error());
        terminate();
        return;
    }

    m_waylandWantsWrite = wantsWrite;
}

void CBackend::enterLoop() {
    {
        std::lock_guard lock(m_loopStateMutex);
        if (m_terminate || !m_eventLoop)
            return;

        m_loopRunning = true;
        m_loopThread  = std::this_thread::get_id();
    }

    if (wl_display_dispatch_pending(g_waylandPlatform->m_waylandState.display) < 0) {
        g_logger->log(HT_LOG_ERROR, "failed to dispatch pending Wayland events: {}", wl_display_get_error(g_waylandPlatform->m_waylandState.display));
        terminate();
    } else
        flushWayland(*m_waylandSource);

    std::expected<void, std::string> result;
    if (!m_terminate)
        result = m_eventLoop->enterLoop();

    {
        std::lock_guard lock(m_loopStateMutex);
        m_loopRunning = false;
        m_terminate   = true;
    }

    if (!result)
        g_logger->log(HT_LOG_ERROR, "event loop failed: {}", result.error());

    cleanup();
}

void CBackend::cleanup() {
    if (m_cleaned.exchange(true))
        return;

    g_asyncResourceGatherer.reset();

    m_waylandPostDispatch.reset();
    m_waylandSource.reset();
    m_configSource.reset();
    m_userFds.clear();
    m_timers.clear();
    ++m_pendingGeneration;
    m_eventLoopExecutor.reset();
    m_eventLoop.reset();

    g_renderer.reset();
    g_openGL.reset();

    g_backendServices.reset();
    g_waylandBackend.reset();
    g_waylandPlatform.reset();
    g_animationManager.reset();
    g_iconFactory.reset();
    g_config.reset();
    g_palette.reset();
    g_logger.reset();

    // Reset this last: it may be the final strong reference to this backend.
    g_backend.reset();
}
