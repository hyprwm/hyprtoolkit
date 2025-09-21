#pragma once

#include <hyprutils/math/Mat3x3.hpp>

#include "../Renderer.hpp"

#include "Shader.hpp"

#include <hyprutils/math/Region.hpp>

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IElement;
    class CGLTexture;

    class COpenGLRenderer : public IRenderer {
      public:
        COpenGLRenderer();
        virtual ~COpenGLRenderer();

        virtual void                 beginRendering(SP<IToolkitWindow> window);
        virtual void                 endRendering();
        virtual void                 renderRectangle(const SRectangleRenderData& data);
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data);
        virtual void                 renderTexture(const STextureRenderData& data);

      private:
        CBox                        logicalToGL(const CBox& box);
        void                        scissor(const CBox& box);
        void                        scissor(const pixman_box32_t* box);

        SP<IToolkitWindow>          m_window;
        CRegion                     m_damage;

        std::vector<SP<CGLTexture>> m_glTextures;

        CShader                     m_rectShader;
        CShader                     m_texShader;

        Mat3x3                      m_projMatrix = Mat3x3::identity();
        Mat3x3                      m_projection;

        float                       m_scale = 1.F;

        Vector2D                    m_currentViewport;
    };

    inline UP<COpenGLRenderer> g_openGL;
}
