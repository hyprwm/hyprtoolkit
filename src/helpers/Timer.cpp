#include <hyprtoolkit/core/Timer.hpp>
#include "Memory.hpp"

using namespace Hyprtoolkit;

CTimer::CTimer(std::chrono::system_clock::duration timeout, std::function<void(ASP<CTimer> self, void* data)> cb_, void* data_, bool force) :
    m_cb(cb_), m_data(data_), m_allowForceUpdate(force) {
    m_expires = std::chrono::system_clock::now() + timeout;
}

bool CTimer::passed() {
    return std::chrono::system_clock::now() > m_expires;
}

void CTimer::cancel() {
    m_wasCancelled = true;
}

bool CTimer::cancelled() {
    return m_wasCancelled;
}

void CTimer::call(ASP<CTimer> self) {
    m_cb(self, m_data);
}

float CTimer::leftMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(m_expires - std::chrono::system_clock::now()).count();
}

bool CTimer::canForceUpdate() {
    return m_allowForceUpdate;
}
