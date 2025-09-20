#pragma once

#include <cstddef>

namespace Hyprtoolkit {
    class IRendererTexture {
      public:
        IRendererTexture()          = default;
        virtual ~IRendererTexture() = default;

        enum eTextureType : uint8_t {
            TEXTURE_GL,
        };

        virtual size_t       id()      = 0;
        virtual eTextureType type()    = 0;
        virtual void         destroy() = 0;
    };
}
