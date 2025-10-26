#include "Sync.hpp"
#include "../../core/InternalBackend.hpp"

using namespace Hyprtoolkit;
using namespace Hyprutils::OS;

UP<CEGLSync> CEGLSync::create() {
    EGLSyncKHR sync = g_openGL->m_proc.eglCreateSyncKHR(g_openGL->m_eglDisplay, EGL_SYNC_NATIVE_FENCE_ANDROID, nullptr);

    if (sync == EGL_NO_SYNC_KHR) {
        g_logger->log(HT_LOG_ERROR, "eglCreateSyncKHR failed");
        return nullptr;
    }

    // we need to flush otherwise we might not get a valid fd
    glFlush();

    int fd = g_openGL->m_proc.eglDupNativeFenceFDANDROID(g_openGL->m_eglDisplay, sync);
    if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID) {
        g_logger->log(HT_LOG_ERROR, "eglDupNativeFenceFDANDROID failed");
        return nullptr;
    }

    UP<CEGLSync> eglSync(new CEGLSync);
    eglSync->m_fd    = CFileDescriptor(fd);
    eglSync->m_sync  = sync;
    eglSync->m_valid = true;

    return eglSync;
}

CEGLSync::~CEGLSync() {
    if (m_sync == EGL_NO_SYNC_KHR)
        return;

    if (g_openGL && g_openGL->m_proc.eglDestroySyncKHR(g_openGL->m_eglDisplay, m_sync) != EGL_TRUE)
        g_logger->log(HT_LOG_ERROR, "eglDestroySyncKHR failed");
}

CFileDescriptor& CEGLSync::fd() {
    return m_fd;
}

CFileDescriptor&& CEGLSync::takeFd() {
    return std::move(m_fd);
}

bool CEGLSync::isValid() {
    return m_valid && m_sync != EGL_NO_SYNC_KHR && m_fd.isValid();
}
