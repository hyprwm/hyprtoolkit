#include "Renderbuffer.hpp"
#include "OpenGL.hpp"
#include "../../core/InternalBackend.hpp"

#include <hyprutils/signal/Listener.hpp>
#include <hyprutils/signal/Signal.hpp>

#include <dlfcn.h>

using namespace Hyprtoolkit;

CRenderbuffer::~CRenderbuffer() {
    g_openGL->makeEGLCurrent();

    unbind();
    m_framebuffer.release();
    glDeleteRenderbuffers(1, &m_rbo);

    g_openGL->m_proc.eglDestroyImageKHR(g_openGL->m_eglDisplay, m_image);
}

CRenderbuffer::CRenderbuffer(SP<Aquamarine::IBuffer> buffer, uint32_t format) : m_hlBuffer(buffer), m_drmFormat(format) {
    auto dma = buffer->dmabuf();

    m_image = g_openGL->createEGLImage(dma);
    if (m_image == EGL_NO_IMAGE_KHR) {
        g_logger->log(HT_LOG_ERROR, "gl: createEGLImage failed for rbo");
        return;
    }

    glGenRenderbuffers(1, &m_rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    g_openGL->m_proc.glEGLImageTargetRenderbufferStorageOES(GL_RENDERBUFFER, m_image);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);

    glGenFramebuffers(1, &m_framebuffer.m_fb);
    m_framebuffer.m_fbAllocated = true;
    m_framebuffer.m_size        = buffer->size;
    m_framebuffer.bind();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        g_logger->log(HT_LOG_ERROR, "rbo: glCheckFramebufferStatus failed");
        return;
    }

    m_framebuffer.unbind();

    m_listeners.destroyBuffer = buffer->events.destroy.listen([this] { g_openGL->onRenderbufferDestroy(this); });

    m_good = true;
}

bool CRenderbuffer::good() {
    return m_good;
}

void CRenderbuffer::bind() {
    glBindRenderbuffer(GL_RENDERBUFFER, m_rbo);
    bindFB();
}

void CRenderbuffer::bindFB() {
    m_framebuffer.bind();
}

void CRenderbuffer::unbind() {
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
    m_framebuffer.unbind();
}

CFramebuffer* CRenderbuffer::getFB() {
    return &m_framebuffer;
}
