#pragma once

#include <hyprtoolkit/palette/Color.hpp>
#include <hyprtoolkit/palette/Gradient.hpp>
#include <hyprtoolkit/types/ImageTypes.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>
#include <aquamarine/buffer/Buffer.hpp>

#include "../helpers/Memory.hpp"

#include <functional>
#include <string>

#include "Polygon.hpp"

using namespace Hyprutils::Math;
using namespace Hyprgraphics;

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IRendererTexture;
    class CSyncTimeline;

    class IRendererShader {
      public:
        virtual ~IRendererShader() = default;
    };

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
            eImageFitMode                     fitMode = IMAGE_FIT_MODE_STRETCH;
        };

        struct STextureRenderData {
            CBox                      box;
            SP<IRendererTexture>      texture;
            float                     a        = 1.F;
            int                       rounding = 0;
            std::optional<CHyprColor> tint;
            bool                      tintGrayscaleOnly = false;
        };

        struct SBorderRenderData {
            CBox               box;
            CGradientValueData gradient;
            int                rounding = 0;
            int                thick    = 0;
        };

        struct SPolygonRenderData {
            CBox       box;
            CHyprColor color = {1, 1, 1, 1};
            CPolygon   poly;
        };

        struct SLineRenderData {
            CBox                        box;
            const std::vector<Vector2D> points;
            CHyprColor                  color = {1, 1, 1, 1};
            int                         thick = 2;
        };

        struct STransitionRenderData {
            CBox                 box;
            SP<IRendererTexture> from, to;
            SP<IRendererShader>  shader; // nullptr = built-in crossfade
            float                progress = 0.F;
            float                a        = 1.F;
            float                duration = 0.F; // seconds, for the shader
            Vector2D             seed;
        };

        virtual void                 beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf) = 0;
        virtual void                 beginRenderingExternal(SP<IToolkitWindow> window, uint32_t bufferAge)  = 0;
        virtual void                 render(bool ignoreSync = false)                                        = 0;
        virtual void                 endRendering()                                                         = 0;
        virtual void                 renderRectangle(const SRectangleRenderData& data)                      = 0;
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data)                                = 0;
        virtual void                 renderTexture(const STextureRenderData& data)                          = 0;
        virtual void                 renderBorder(const SBorderRenderData& data)                            = 0;
        virtual void                 renderPolygon(const SPolygonRenderData& data)                          = 0;
        virtual void                 renderLine(const SLineRenderData& data)                                = 0;
        virtual void                 signalRenderPoint(SP<CSyncTimeline> timeline)                          = 0;

        virtual SP<CSyncTimeline>    exportSync(SP<Aquamarine::IBuffer> buf) = 0;

        virtual SP<IRendererShader>  createTransitionShader(const std::string& fragSource)                       = 0; // nullptr on failure (logged)
        virtual void                 renderTransition(const STransitionRenderData& data)                         = 0;
        virtual SP<IRendererTexture> renderToTexture(const Vector2D& pixelSize, const std::function<void()>& fn) = 0;

        virtual bool                 explicitSyncSupported() = 0;

        virtual int                  getMaxTextureSize() = 0;
    };

    inline SP<IRenderer> g_renderer;
}
