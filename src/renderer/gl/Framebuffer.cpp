#include "Framebuffer.hpp"
#include "OpenGL.hpp"
#include "GLTexture.hpp"
#include "../../core/InternalBackend.hpp"
#include "GL.hpp"

#include <GLES3/gl32.h>

#include "../../helpers/Format.hpp"
#include "../../Macros.hpp"

using namespace Hyprtoolkit;

CFramebuffer::CFramebuffer() {
    ;
}

bool CFramebuffer::alloc(int w, int h, uint32_t drmFormat) {
    bool firstAlloc = false;
    RASSERT((w > 0 && h > 0), "cannot alloc a FB with negative / zero size! (attempted {}x{})", w, h);

    uint32_t glFormat = NFormatUtils::drmFormatToGL(drmFormat);
    uint32_t glType   = NFormatUtils::glFormatToType(glFormat);

    if (drmFormat != m_drmFormat || m_size != Vector2D{w, h})
        release();

    m_drmFormat = drmFormat;

    if (!m_tex) {
        m_tex = makeShared<CGLTexture>();
        m_tex->allocate();
        m_tex->bind();
        glTexParameteri(m_tex->m_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(m_tex->m_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(m_tex->m_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(m_tex->m_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        firstAlloc = true;
    }

    if (!m_fbAllocated) {
        glGenFramebuffers(1, &m_fb);
        m_fbAllocated = true;
        firstAlloc    = true;
    }

    if (firstAlloc || m_size != Vector2D(w, h)) {
        m_tex->bind();
        glTexImage2D(GL_TEXTURE_2D, 0, glFormat, w, h, 0, GL_RGBA, glType, nullptr);
        glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_tex->m_texID, 0);

        auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        RASSERT((status == GL_FRAMEBUFFER_COMPLETE), "Framebuffer incomplete, couldn't create! (FB status: {}, GL Error: 0x{:x})", status, sc<int>(glGetError()));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    m_size = Vector2D(w, h);

    return true;
}

void CFramebuffer::bind() {
    GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_fb));
}

void CFramebuffer::unbind() {
    GLCALL(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
}

void CFramebuffer::release() {
    if (m_fbAllocated) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_fb);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteFramebuffers(1, &m_fb);
        m_fbAllocated = false;
        m_fb          = 0;
    }

    if (m_tex)
        m_tex.reset();

    m_size = Vector2D();
}

CFramebuffer::~CFramebuffer() {
    release();
}

bool CFramebuffer::isAllocated() {
    return m_fbAllocated && m_tex;
}

SP<CGLTexture> CFramebuffer::getTexture() {
    return m_tex;
}

GLuint CFramebuffer::getFBID() {
    return m_fbAllocated ? m_fb : 0;
}
