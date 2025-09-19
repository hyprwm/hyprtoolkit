#pragma once

#include <hyprutils/math/Mat3x3.hpp>

#include "../Renderer.hpp"

#include "Shader.hpp"

#include <functional>

namespace Hyprtoolkit {
    class IWindow;
    class IElement;

    class COpenGLRenderer : public IRenderer {
      public:
        COpenGLRenderer();
        virtual ~COpenGLRenderer();

        virtual void beginRendering(SP<IWindow> window);

        virtual void renderRectangle(const SRectangleRenderData& data);

      private:
        void     bfHelper(std::vector<SP<IElement>> elements, const std::function<void(SP<IElement>)>& fn);
        void     breadthfirst(SP<IElement> element, const std::function<void(SP<IElement>)>& fn);

        CBox     logicalToGL(const CBox& box);

        CShader  m_rectShader;

        Mat3x3   m_projMatrix = Mat3x3::identity();
        Mat3x3   m_projection;

        float    m_scale = 1.F;

        Vector2D m_currentViewport;
    };

    inline UP<COpenGLRenderer> g_openGL;
}
