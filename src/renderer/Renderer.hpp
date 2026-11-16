#pragma once

#include <hyprtoolkit/palette/Color.hpp>
#include <hyprtoolkit/palette/Gradient.hpp>
#include <hyprtoolkit/types/ImageTypes.hpp>
#include <hyprutils/math/Box.hpp>
#include <hyprgraphics/color/Color.hpp>
#include <hyprgraphics/resource/resources/AsyncResource.hpp>
#include <aquamarine/buffer/Buffer.hpp>
#include <optional>

#include "../helpers/Memory.hpp"

#include "Polygon.hpp"

using namespace Hyprutils::Math;
using namespace Hyprgraphics;

namespace Hyprtoolkit {
    class IToolkitWindow;
    class IRendererTexture;
    class CSyncTimeline;
    class CFramebuffer;

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
            CBox                         box;
            SP<IRendererTexture>         texture;
            float                        a        = 1.F;
            int                          rounding = 0;
            std::optional<CHyprColor>    tint;
            bool                         tintGrayscaleOnly = false;
            std::optional<eImageFitMode> fitMode; // if unset, falls back to the texture's stored fit mode
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
            CBox                                 box;
            SP<IRendererTexture>                 startTexture;
            SP<IRendererTexture>                 endTexture;
            float                                progress = 0.0f;
            float                                a = 1.F;
            int                                  rounding = 0;
            eImageFitMode                        fitMode = IMAGE_FIT_MODE_COVER;
            size_t                               shaderKey = 0;
            Vector2D                             randomPixel;
            float                                duration = 0.0f;  // passed to shader via u_duration
        };

        virtual void                 beginRendering(SP<IToolkitWindow> window, SP<Aquamarine::IBuffer> buf) = 0;
        virtual void                 beginRenderingExternal(SP<IToolkitWindow> window, uint32_t bufferAge)  = 0;
        virtual void                 render(bool ignoreSync = false)                                        = 0;
        virtual void                 endRendering()                                                         = 0;
        virtual void                 renderRectangle(const SRectangleRenderData& data)                      = 0;
        virtual SP<IRendererTexture> uploadTexture(const STextureData& data)                                = 0;
        virtual void                 renderTexture(const STextureRenderData& data)                          = 0;
        virtual void                 renderTransition(const STransitionRenderData& data)                      = 0;
        virtual void                 renderBorder(const SBorderRenderData& data)                            = 0;
        virtual void                 renderPolygon(const SPolygonRenderData& data)                          = 0;
        virtual SP<IRendererTexture> captureTransitionState(const STransitionRenderData& data)                = 0;
        // Pre-render a single texture to a fit-resolved offscreen FBO and return the
        // resolved texture. Cached; the transition shaders are pure crossfades.
        virtual SP<IRendererTexture> renderFitFrame(const SP<IRendererTexture>& src, const CBox& box, eImageFitMode fitMode) = 0;
        // Drop all cached fit-resolved pre-render FBOs (call when a transition starts/ends).
        virtual void                 clearTransitionCache()                                                       = 0;
        // Compile a transition shader from raw GLSL source and cache it. Returns the cache key.
        virtual size_t               ensureTransitionShader(const std::string& source)                        = 0;
        virtual void                 renderLine(const SLineRenderData& data)                                = 0;
        virtual void                 signalRenderPoint(SP<CSyncTimeline> timeline)                          = 0;

        virtual SP<CSyncTimeline>    exportSync(SP<Aquamarine::IBuffer> buf) = 0;

        virtual bool                 explicitSyncSupported() = 0;

        virtual int                  getMaxTextureSize() = 0;
    };

    inline SP<IRenderer> g_renderer;
}
