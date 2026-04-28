#pragma once

#include <hyprutils/math/Vector2D.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>

#include "GL.hpp"

#include <cstdint>

#include "../RendererTexture.hpp"
#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    enum eGLTextureType : uint8_t {
        TEXTURE_INVALID,  // Invalid
        TEXTURE_RGBA,     // 4 channels
        TEXTURE_RGBX,     // discard A
        TEXTURE_EXTERNAL, // EGLImage
    };

    class CTimer;

    class CGLTexture : public IRendererTexture {
      public:
        CGLTexture();
        virtual ~CGLTexture();

        virtual size_t                    id();
        virtual eTextureType              type();
        virtual void                      destroy();
        virtual eImageFitMode             fitMode();
        virtual Hyprutils::Math::Vector2D size();

        // attaches an async resource. uploads immediately if ready, otherwise
        // arms a listener that runs upload() when the resource is ready. self is
        // captured weakly so the listener becomes a no-op if the texture is
        // destroyed before the resource finishes loading.
        void                              attachAsync(WP<CGLTexture> self, ASP<Hyprgraphics::IAsyncResource> resource);

        eGLTextureType                    m_type      = TEXTURE_RGBA;
        GLenum                            m_target    = GL_TEXTURE_2D;
        bool                              m_allocated = false;
        GLuint                            m_texID     = 0;
        eImageFitMode                     m_fitMode   = IMAGE_FIT_MODE_STRETCH;
        Hyprutils::Math::Vector2D         m_size      = {};

        ASP<Hyprgraphics::IAsyncResource> m_resource;

        void                              upload();
        void                              allocate();
        void                              bind();
    };
};