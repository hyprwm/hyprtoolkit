#include <hyprtoolkit/core/Timer.hpp>
#include "Memory.hpp"
#include "../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

CTimer::CTimer(std::chrono::steady_clock::duration timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data_, bool force) :
    m_cb(cb_), m_data(data_), m_allowForceUpdate(force) {
    m_expires = std::chrono::steady_clock::now() + timeout;
}

bool CTimer::passed() {
    return std::chrono::steady_clock::now() > m_expires;
}

void CTimer::cancel() {
    m_wasCancelled = true;

    if (const auto backend = g_waylandBackend.lock())
        backend->cancelTimer(this);
}

bool CTimer::cancelled() {
    return m_wasCancelled;
}

void CTimer::call(ASP<CTimer> self) {
    m_cb(self, m_data);
}

void CTimer::updateTimeout(std::chrono::steady_clock::duration timeout) {
    m_expires = std::chrono::steady_clock::now() + timeout;

    if (const auto backend = g_waylandBackend.lock())
        backend->updateTimer(this, m_expires);
}

float CTimer::leftMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(m_expires - std::chrono::steady_clock::now()).count();
}

bool CTimer::canForceUpdate() {
    return m_allowForceUpdate;
}
