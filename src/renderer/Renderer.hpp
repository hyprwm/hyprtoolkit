#pragma once

#include <hyprutils/math/Box.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>

#include "../helpers/Memory.hpp"

using namespace Hyprutils::Math;
using namespace Hyprgraphics;

namespace Hyprtoolkit {
    class IWindow;
    class IRendererTexture;

    class IRenderer {
      public:
        IRenderer()          = default;
        virtual ~IRenderer() = default;

        struct SRectangleRenderData {
            CBox   box;
            CColor color{{.r = 1.F, .g = 1.F, .b = 1.F}};
            float  a        = 1.F;
            int    rounding = 0;
        };

        struct STextureData {
            ASP<Hyprgraphics::IAsyncResource> resource;
        };

        struct STextureRenderData {
            CBox                 box;
            SP<IRendererTexture> texture;
            float                a        = 1.F;
            int                  rounding = 0;
        };

        virtual void                 beginRendering(SP<IWindow> window)                = 0;
        virtual void                 renderRectangle(const SRectangleRenderData& data) = 0;
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data)           = 0;
        virtual void                 renderTexture(const STextureRenderData& data)     = 0;
    };

    inline UP<IRenderer> g_renderer;
}
