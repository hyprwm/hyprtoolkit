#include "SyncTimeline.hpp"
#include "../../Macros.hpp"
#include "../../core/InternalBackend.hpp"
#include "../Renderer.hpp"

#include <xf86drm.h>
#include <sys/eventfd.h>

using namespace Hyprutils::OS;
using namespace Hyprtoolkit;

SP<CSyncTimeline> CSyncTimeline::create(int drmFD_) {
    RASSERT(g_renderer->explicitSyncSupported(), "Tried to create a sync timeline without ES support");

    auto timeline     = SP<CSyncTimeline>(new CSyncTimeline);
    timeline->m_drmFD = drmFD_;
    timeline->m_self  = timeline;

    if (drmSyncobjCreate(drmFD_, 0, &timeline->m_handle)) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline: failed to create a drm syncobj??");
        return nullptr;
    }

    int objFd = -1;
    if (drmSyncobjHandleToFD(drmFD_, timeline->m_handle, &objFd)) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline: failed to get a syncobj fd??");
        return nullptr;
    }

    timeline->m_syncobjFD = Hyprutils::OS::CFileDescriptor(objFd);

    return timeline;
}

SP<CSyncTimeline> CSyncTimeline::create(int drmFD_, CFileDescriptor&& drmSyncobjFD) {
    RASSERT(g_renderer->explicitSyncSupported(), "Tried to create a sync timeline without ES support");

    auto timeline         = SP<CSyncTimeline>(new CSyncTimeline);
    timeline->m_drmFD     = drmFD_;
    timeline->m_syncobjFD = std::move(drmSyncobjFD);
    timeline->m_self      = timeline;

    if (drmSyncobjFDToHandle(drmFD_, timeline->m_syncobjFD.get(), &timeline->m_handle)) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline: failed to create a drm syncobj from fd??");
        return nullptr;
    }

    return timeline;
}

CSyncTimeline::~CSyncTimeline() {
    if (m_handle == 0)
        return;

    drmSyncobjDestroy(m_drmFD, m_handle);
}

std::optional<bool> CSyncTimeline::check(uint64_t point, uint32_t flags) {
#ifdef __FreeBSD__
    constexpr int ETIME_ERR = ETIMEDOUT;
#else
    constexpr int ETIME_ERR = ETIME;
#endif

    uint32_t signaled = 0;
    int      ret      = drmSyncobjTimelineWait(m_drmFD, &m_handle, &point, 1, 0, flags, &signaled);
    if (ret != 0 && ret != -ETIME_ERR) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::check: drmSyncobjTimelineWait failed");
        return std::nullopt;
    }

    return ret == 0;
}

bool CSyncTimeline::addWaiter(std::function<void()>&& waiter, uint64_t point, uint32_t flags) {
    auto eventFd = CFileDescriptor(eventfd(0, EFD_CLOEXEC));

    if (!eventFd.isValid()) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::addWaiter: failed to acquire an eventfd");
        return false;
    }

    if (drmSyncobjEventfd(m_drmFD, m_handle, point, eventFd.get(), flags)) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::addWaiter: drmSyncobjEventfd failed");
        return false;
    }

    g_backend->doOnReadable(std::move(eventFd), std::move(waiter));

    return true;
}

CFileDescriptor CSyncTimeline::exportAsSyncFileFD(uint64_t src) {
    int      sync = -1;

    uint32_t syncHandle = 0;
    if (drmSyncobjCreate(m_drmFD, 0, &syncHandle)) {
        g_logger->log(HT_LOG_ERROR, "exportAsSyncFileFD: drmSyncobjCreate failed");
        return {};
    }

    if (drmSyncobjTransfer(m_drmFD, syncHandle, 0, m_handle, src, 0)) {
        g_logger->log(HT_LOG_ERROR, "exportAsSyncFileFD: drmSyncobjTransfer failed");
        drmSyncobjDestroy(m_drmFD, syncHandle);
        return {};
    }

    if (drmSyncobjExportSyncFile(m_drmFD, syncHandle, &sync)) {
        g_logger->log(HT_LOG_ERROR, "exportAsSyncFileFD: drmSyncobjExportSyncFile failed");
        drmSyncobjDestroy(m_drmFD, syncHandle);
        return {};
    }

    drmSyncobjDestroy(m_drmFD, syncHandle);
    return CFileDescriptor{sync};
}

bool CSyncTimeline::importFromSyncFileFD(uint64_t dst, CFileDescriptor& fd) {
    uint32_t syncHandle = 0;

    if (drmSyncobjCreate(m_drmFD, 0, &syncHandle)) {
        g_logger->log(HT_LOG_ERROR, "importFromSyncFileFD: drmSyncobjCreate failed");
        return false;
    }

    if (drmSyncobjImportSyncFile(m_drmFD, syncHandle, fd.get())) {
        g_logger->log(HT_LOG_ERROR, "importFromSyncFileFD: drmSyncobjImportSyncFile failed");
        drmSyncobjDestroy(m_drmFD, syncHandle);
        return false;
    }

    if (drmSyncobjTransfer(m_drmFD, m_handle, dst, syncHandle, 0, 0)) {
        g_logger->log(HT_LOG_ERROR, "importFromSyncFileFD: drmSyncobjTransfer failed");
        drmSyncobjDestroy(m_drmFD, syncHandle);
        return false;
    }

    drmSyncobjDestroy(m_drmFD, syncHandle);
    return true;
}

bool CSyncTimeline::transfer(SP<CSyncTimeline> from, uint64_t fromPoint, uint64_t toPoint) {
    if (m_drmFD != from->m_drmFD) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::transfer: cannot transfer timelines between gpus, {} -> {}", from->m_drmFD, m_drmFD);
        return false;
    }

    if (drmSyncobjTransfer(m_drmFD, m_handle, toPoint, from->m_handle, fromPoint, 0)) {
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::transfer: drmSyncobjTransfer failed");
        return false;
    }

    return true;
}

void CSyncTimeline::signal(uint64_t point) {
    if (drmSyncobjTimelineSignal(m_drmFD, &m_handle, &point, 1))
        g_logger->log(HT_LOG_ERROR, "CSyncTimeline::signal: drmSyncobjTimelineSignal failed");
}
