#pragma once

#include <hyprutils/math/Box.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>

#include "../helpers/Memory.hpp"
#include "../helpers/Color.hpp"

using namespace Hyprutils::Math;
using namespace Hyprgraphics;

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IRendererTexture;

    class IRenderer {
      public:
        IRenderer()          = default;
        virtual ~IRenderer() = default;

        struct SRectangleRenderData {
            CBox       box;
            CHyprColor color;
            int        rounding = 0;
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

        struct SBorderRenderData {
            CBox       box;
            CHyprColor color    = {1, 1, 1, 1};
            int        rounding = 0;
            int        thick    = 0;
        };

        virtual void                 beginRendering(SP<IToolkitWindow> window)         = 0;
        virtual void                 endRendering()                                    = 0;
        virtual void                 renderRectangle(const SRectangleRenderData& data) = 0;
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data)           = 0;
        virtual void                 renderTexture(const STextureRenderData& data)     = 0;
        virtual void                 renderBorder(const SBorderRenderData& data)       = 0;
    };

    inline UP<IRenderer> g_renderer;
}
