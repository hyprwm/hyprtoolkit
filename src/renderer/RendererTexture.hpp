#pragma once

#include <cstddef>

#include <hyprtoolkit/types/ImageTypes.hpp>
#include <hyprutils/math/Vector2D.hpp>

namespace Hyprtoolkit {
    class IRendererTexture {
      public:
        IRendererTexture()          = default;
        virtual ~IRendererTexture() = default;

        enum eTextureType : uint8_t {
            TEXTURE_GL,
        };

        virtual size_t                    id()      = 0;
        virtual eTextureType              type()    = 0;
        virtual void                      destroy() = 0;
        virtual eImageFitMode             fitMode() = 0;
        virtual Hyprutils::Math::Vector2D size()    = 0;
    };
}
