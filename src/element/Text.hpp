
#pragma once

#include <hyprtoolkit/element/Text.hpp>

namespace Hyprtoolkit {
    struct STextImpl {
        size_t                                                               lastFontSizeUnscaled = 0;
        float                                                                lastScale            = 1.F;
        bool                                                                 needsTexRefresh      = false;

        Hyprutils::Math::Vector2D                                            lastMaxSize;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                  tex;
        Hyprutils::Memory::CSharedPointer<IRendererTexture>                  oldTex; // while loading a new one
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CTextResource> resource;
        Hyprutils::Math::Vector2D                                            size;

        bool                                                                 waitingForTex = false;
    };
}
