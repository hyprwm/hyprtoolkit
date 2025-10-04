
#pragma once

#include <hyprtoolkit/element/Text.hpp>

#include "../../helpers/Memory.hpp"
#include "../../core/InternalBackend.hpp"

namespace Hyprtoolkit {
    struct STextData {
        std::string                              text;
        std::string                              fontFamily = "Sans Serif";
        CFontSize                                fontSize{CFontSize::HT_FONT_TEXT};
        eFontAlignment                           align       = HT_FONT_ALIGN_LEFT;
        colorFn                                  color       = [] { return g_backend->getPalette()->m_colors.text; };
        float                                    a           = 1.F;
        bool                                     noEllipsize = false;
        std::optional<Hyprutils::Math::Vector2D> clampSize;
        CDynamicSize                             size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
        std::function<void()>                    callback; // called after resource is loaded
    };

    struct STextImpl {
        STextData                                                            data;

        WP<CTextElement>                                                     self;

        size_t                                                               lastFontSizeUnscaled = 0;
        float                                                                lastScale            = 1.F;
        bool                                                                 needsTexRefresh = false, newTex = false;

        Hyprutils::Math::Vector2D                                            lastMaxSize;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                  tex;
        Hyprutils::Memory::CSharedPointer<IRendererTexture>                  oldTex; // while loading a new one
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CTextResource> resource;
        Hyprutils::Math::Vector2D                                            size, preferred;

        bool                                                                 waitingForTex = false;

        Hyprutils::Math::Vector2D                                            getTextSizePreferred();
        Hyprutils::Math::Vector2D                                            unscale(const Hyprutils::Math::Vector2D& x);
    };
}
