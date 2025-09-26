#include "GLTexture.hpp"
#include "OpenGL.hpp"

#include "../../core/InternalBackend.hpp"

using namespace Hyprtoolkit;

CGLTexture::CGLTexture(ASP<Hyprgraphics::IAsyncResource> resource) {
    if (resource->m_ready) {
        m_resource = resource;
        upload();
        return;
    }

    // not ready yet, add a timer when it is and do it
    // FIXME: could UAF. Maybe keep wref?
    resource->m_events.finished.listenStatic([this, resource] {
        g_backend->addIdle([this, resource]() {
            m_resource = resource;
            upload();
        });
    });
}

CGLTexture::CGLTexture() {
    ;
}

CGLTexture::~CGLTexture() {
    destroy();
}

void CGLTexture::upload() {
    const cairo_status_t SURFACESTATUS = (cairo_status_t)m_resource->m_asset.cairoSurface->status();
    const auto           CAIROFORMAT   = cairo_image_surface_get_format(m_resource->m_asset.cairoSurface->cairo());
    const GLint          glIFormat     = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB32F : GL_RGBA;
    const GLint          glFormat      = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_RGB : GL_RGBA;
    const GLint          glType        = CAIROFORMAT == CAIRO_FORMAT_RGB96F ? GL_FLOAT : GL_UNSIGNED_BYTE;

    allocate();

    if (SURFACESTATUS != CAIRO_STATUS_SUCCESS) {
        g_logger->log(HT_LOG_ERROR, "Resource {} invalid: failed to load, renderer will ignore");
        m_type = TEXTURE_INVALID;
        return;
    }

    m_type = TEXTURE_RGBA;
    m_size = m_resource->m_asset.pixelSize;

    GLCALL(glBindTexture(GL_TEXTURE_2D, m_texID));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    if (CAIROFORMAT != CAIRO_FORMAT_RGB96F) {
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE));
        GLCALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED));
    }
    GLCALL(glTexImage2D(GL_TEXTURE_2D, 0, glIFormat, m_size.x, m_size.y, 0, glFormat, glType, m_resource->m_asset.cairoSurface->data()));

    m_resource.reset();
}

size_t CGLTexture::id() {
    return m_texID;
}

IRendererTexture::eTextureType CGLTexture::type() {
    return TEXTURE_GL;
}

void CGLTexture::destroy() {
    g_openGL->makeEGLCurrent();
    if (m_allocated) {
        GLCALL(glDeleteTextures(1, &m_texID));
        m_texID = 0;
    }
    m_allocated = false;
}

void CGLTexture::allocate() {
    if (!m_allocated)
        GLCALL(glGenTextures(1, &m_texID));
    m_allocated = true;
}

void CGLTexture::bind() {
    GLCALL(glBindTexture(m_target, m_texID));
}
