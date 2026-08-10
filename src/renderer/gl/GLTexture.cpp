#include "GLTexture.hpp"
#include "OpenGL.hpp"

#include "../../core/InternalBackend.hpp"
#include "../../core/BackendContext.hpp"

using namespace Hyprtoolkit;

CGLTexture::CGLTexture() {
    if (g_backendServices)
        m_lifetime = g_backendServices->lifetime;
}

void CGLTexture::attachAsync(WP<CGLTexture> self, ASP<Hyprgraphics::IAsyncResource> resource) {
    if (resource->m_ready) {
        m_resource = resource;
        uploadOnContext(self);
        return;
    }

    resource->m_events.finished.listenStatic([self, resource, lifetime = WP<SBackendLifetime>{g_backendServices->lifetime}] {
        // backend may have been torn down between enqueue and finish (shutdown
        // race). dropping the upload is safe: the texture is either gone too
        // (lock fails below) or about to be.
        if (!g_backend || !lifetime)
            return;
        g_backend->addIdle([self, resource, lifetime] {
            const auto SELF = self.lock();
            if (!SELF || !lifetime)
                return;
            SELF->m_resource = resource;
            SELF->uploadOnContext(SELF);
        });
    });
}

void CGLTexture::uploadOnContext(WP<CGLTexture> self) {
    if (g_openGL && g_openGL->m_borrowedContext && !g_openGL->m_window) {
        g_openGL->enqueueGL([self] {
            if (const auto locked = self.lock())
                locked->upload();
        });
        return;
    }

    if (g_openGL)
        g_openGL->makeEGLCurrent();
    upload();
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
    if (!m_lifetime) {
        m_texID     = 0;
        m_allocated = false;
        return;
    }

    if (m_allocated && g_openGL && g_openGL->m_borrowedContext && !g_openGL->contextCurrent()) {
        const auto texture = m_texID;
        g_openGL->enqueueGL([texture] { glDeleteTextures(1, &texture); });
        m_texID     = 0;
        m_allocated = false;
        return;
    }

    if (g_openGL)
        g_openGL->makeEGLCurrent();

    if (m_allocated) {
        GLCALL(glDeleteTextures(1, &m_texID));
        m_texID = 0;
    }
    m_allocated = false;
}

void CGLTexture::releaseFromRenderer() {
    if (m_allocated)
        glDeleteTextures(1, &m_texID);
    m_texID     = 0;
    m_allocated = false;
    m_type      = TEXTURE_INVALID;
}

void CGLTexture::allocate() {
    if (!m_allocated)
        GLCALL(glGenTextures(1, &m_texID));
    m_allocated = true;
}

void CGLTexture::bind() {
    GLCALL(glBindTexture(m_target, m_texID));
}

eImageFitMode CGLTexture::fitMode() {
    return m_fitMode;
}

Vector2D CGLTexture::size() {
    return m_size;
}
