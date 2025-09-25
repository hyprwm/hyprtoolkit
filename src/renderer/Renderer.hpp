#pragma once

#include <hyprtoolkit/palette/Color.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>
#include <aquamarine/buffer/Buffer.hpp>

#include "../helpers/Memory.hpp"

#include "Polygon.hpp"

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

        struct SPolygonRenderData {
            CBox       box;
            CHyprColor color = {1, 1, 1, 1};
            CPolygon   poly;
        };

        virtual void                 beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf) = 0;
        virtual void                 endRendering()                                                         = 0;
        virtual void                 renderRectangle(const SRectangleRenderData& data)                      = 0;
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data)                                = 0;
        virtual void                 renderTexture(const STextureRenderData& data)                          = 0;
        virtual void                 renderBorder(const SBorderRenderData& data)                            = 0;
        virtual void                 renderPolygon(const SPolygonRenderData& data)                          = 0;
    };

    inline SP<IRenderer> g_renderer;
}
