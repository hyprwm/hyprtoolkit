#include "EmbeddedBackend.hpp"

#include "AnimationManager.hpp"
#include "BackendContext.hpp"
#include "InternalBackend.hpp"
#include "Logger.hpp"
#include "../palette/ConfigManager.hpp"
#include "../renderer/gl/OpenGL.hpp"
#include "../system/Icons.hpp"
#include "../window/EmbeddedSurface.hpp"

using namespace Hyprtoolkit;

SP<IEmbeddedBackend> IEmbeddedBackend::create(const SCreationData& data) {
    if (g_backend || !data.eventLoop)
        return nullptr;

    g_logger                     = makeShared<CLogger>();
    g_logger->m_loggerConnection = data.common.pLogConnection;
    g_logger->updateLogLevel();

    auto backend = makeShared<CEmbeddedBackend>(data);
    g_backend    = backend;

    backend->m_outputAddedListener = g_backendServices->outputs->m_events.outputAdded.listen([weak = WP<CEmbeddedBackend>{backend}](SP<IOutput> output) {
        if (const auto locked = weak.lock())
            if (!locked->m_destroyed)
                locked->m_events.outputAdded.emit(output);
    });

    g_config = makeShared<CConfigManager>();
    g_config->parse();
    g_palette               = CPalette::palette();
    g_iconFactory           = SP<CSystemIconFactory>(new CSystemIconFactory());
    g_asyncResourceGatherer = makeShared<Hyprgraphics::CAsyncResourceGatherer>();
    g_animationManager      = makeShared<CHTAnimationManager>();
    g_openGL                = makeShared<COpenGLRenderer>();
    g_renderer              = g_openGL;

    return backend;
}

CEmbeddedBackend::CEmbeddedBackend(const SCreationData& data) {
    g_backendServices              = makeUnique<SBackendServices>(makeDefaultBackendServices());
    g_backendServices->eventLoop   = data.eventLoop;
    g_backendServices->clipboard   = data.clipboard ? data.clipboard : g_backendServices->clipboard;
    g_backendServices->textInput   = data.textInput ? data.textInput : g_backendServices->textInput;
    g_backendServices->cursor      = data.cursor ? data.cursor : g_backendServices->cursor;
    g_backendServices->outputs     = data.outputs ? data.outputs : g_backendServices->outputs;
    g_backendServices->sessionLock = data.sessionLock ? data.sessionLock : g_backendServices->sessionLock;
}

CEmbeddedBackend::~CEmbeddedBackend() {
    destroy();
}

void CEmbeddedBackend::destroy() {
    if (m_destroyed || m_destroyRequested)
        return;

    if (m_activeFrames > 0) {
        m_destroyRequested = true;
        return;
    }

    destroyNow();
}

void CEmbeddedBackend::destroyNow() {
    if (m_destroyed)
        return;

    const auto keepAlive = g_backend;
    (void)keepAlive;
    m_destroyed        = true;
    m_destroyRequested = false;

    g_backendServices->eventLoop->cancelPending();
    m_outputAddedListener.reset();

    for (const auto& surface : m_surfaces) {
        if (surface)
            surface->invalidate();
    }
    m_surfaces.clear();

    g_asyncResourceGatherer.reset();
    g_animationManager.reset();
    g_iconFactory.reset();
    g_config.reset();
    g_palette.reset();

    g_renderer.reset();
    g_openGL.reset();

    g_backendServices.reset();
    g_logger.reset();
    g_backend.reset();
}

void CEmbeddedBackend::setLogFn(LogFn&& fn) {
    if (m_destroyed || !g_logger)
        return;
    g_logger->m_logFn = std::move(fn);
}

void CEmbeddedBackend::addFd(int fd, std::function<void()>&& callback) {
    if (m_destroyed)
        return;
    g_backendServices->eventLoop->addFd(fd, std::move(callback));
}

void CEmbeddedBackend::removeFd(int fd) {
    if (m_destroyed)
        return;
    g_backendServices->eventLoop->removeFd(fd);
}

SP<ISystemIconFactory> CEmbeddedBackend::systemIcons() {
    return g_iconFactory;
}

ASP<CTimer> CEmbeddedBackend::addTimer(const TimerDuration& timeout, std::function<void(ASP<CTimer> self, void* data)> callback, void* data, bool force) {
    if (m_destroyed)
        return nullptr;
    return g_backendServices->eventLoop->addTimer(timeout, std::move(callback), data, force);
}

void CEmbeddedBackend::enterLoop() {
    if (m_destroyed)
        return;
    g_backendServices->eventLoop->enterLoop();
}

void CEmbeddedBackend::addIdle(const std::function<void()>& callback) {
    if (m_destroyed)
        return;
    g_backendServices->eventLoop->addIdle(callback);
}

SP<CPalette> CEmbeddedBackend::getPalette() {
    return g_palette;
}

std::vector<SP<IOutput>> CEmbeddedBackend::getOutputs() {
    if (m_destroyed)
        return {};
    return g_backendServices->outputs->outputs();
}

std::expected<SP<ISessionLockState>, eSessionLockError> CEmbeddedBackend::aquireSessionLock() {
    if (m_destroyed)
        return std::unexpected(LOCK_ERROR_PLATFORM_UNINITIALIZED);
    return g_backendServices->sessionLock->acquire();
}

SP<IEmbeddedSurface> CEmbeddedBackend::createSurface() {
    if (m_destroyed || m_destroyRequested)
        return nullptr;

    auto surface = makeShared<CEmbeddedSurface>();
    surface->setSelf(surface, dynamicPointerCast<CEmbeddedBackend>(g_backend));
    surface->open();
    m_surfaces.emplace_back(surface);
    return dynamicPointerCast<IEmbeddedSurface>(surface);
}

bool CEmbeddedBackend::beginFrame() {
    if (m_destroyed || m_destroyRequested)
        return false;
    m_activeFrames++;
    return true;
}

void CEmbeddedBackend::endFrame() {
    if (m_activeFrames == 0)
        return;
    m_activeFrames--;
    if (m_activeFrames == 0 && m_destroyRequested)
        destroyNow();
}
