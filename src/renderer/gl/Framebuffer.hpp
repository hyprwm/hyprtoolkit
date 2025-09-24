#pragma once

#include "GLTexture.hpp"

namespace Hyprtoolkit {

    class CFramebuffer {
      public:
        CFramebuffer();
        ~CFramebuffer();

        bool                      alloc(int w, int h, uint32_t format = GL_RGBA);
        void                      bind();
        void                      unbind();
        void                      release();
        void                      reset();
        bool                      isAllocated();
        SP<CGLTexture>            getTexture();
        GLuint                    getFBID();

        Hyprutils::Math::Vector2D m_size;
        uint32_t                  m_drmFormat = 0 /* DRM_FORMAT_INVALID */;

      private:
        SP<CGLTexture> m_tex;
        GLuint         m_fb          = -1;
        bool           m_fbAllocated = false;

        friend class CRenderbuffer;
    };
}
