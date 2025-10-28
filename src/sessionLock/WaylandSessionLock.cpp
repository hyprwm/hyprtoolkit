#include "WaylandSessionLock.hpp"
#include "../core/InternalBackend.hpp"
#include "../core/platforms/WaylandPlatform.hpp"
#include "../window/WaylandLockSurface.hpp"

using namespace Hyprtoolkit;

CWaylandSessionLockState::CWaylandSessionLockState(SP<CCExtSessionLockV1> lock) : m_lock(lock) {
    m_lock->setLocked([this](CCExtSessionLockV1* r) { m_sessionLocked = true; });
    m_lock->setFinished([this](CCExtSessionLockV1* r) {
        g_logger->log(HT_LOG_ERROR, "We got denied by the compositor to be the exclusive lock screen client. Is there another lock screen active?");
        m_denied = true;
        m_events.finished.emit();
    });
}

void CWaylandSessionLockState::unlock() {
    if (m_sessionUnlocked) {
        g_logger->log(HT_LOG_WARNING, "Double unlock in WaylandSessionLockState!");
        return;
    }

    if (!m_lock) {
        g_logger->log(HT_LOG_WARNING, "Unlock without an active sessionLock object in WaylandSessionLockState!");
        return;
    }

    m_lock->sendUnlockAndDestroy();
    m_lock.reset();
    m_sessionUnlocked = true;

    // roundtrip in order to make sure we have unlocked before sending closeRequest
    wl_display_roundtrip(g_waylandPlatform->m_waylandState.display);

    for (const auto& sls : m_lockSurfaces) {
        if (sls.expired())
            continue;
        sls->m_events.closeRequest.emit();
    }

    m_lockSurfaces.clear();
}

void CWaylandSessionLockState::onOutputRemoved(uint32_t outputHandle) {
    auto lockSurfaceIt = std::ranges::find_if(m_lockSurfaces, [outputHandle](const auto& other) { return other->m_outputHandle == outputHandle; });
    if (lockSurfaceIt != m_lockSurfaces.end()) {
        (*lockSurfaceIt)->m_events.closeRequest.emit();
        m_lockSurfaces.erase(lockSurfaceIt);
    }
}
