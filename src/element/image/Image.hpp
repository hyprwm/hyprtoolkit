#include <hyprtoolkit/element/Image.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    struct SImageData {
        std::string                path;
        float                      a        = 1.F;
        int                        rounding = 0;
        SP<ISystemIconDescription> icon;
        CDynamicSize               size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };

    struct SImageImpl {
        SImageData                                                            data;

        WP<CImageElement>                                                     self;

        float                                                                 lastScale = 1.F;

        Hyprutils::Memory::CSharedPointer<IRendererTexture>                   tex;
        Hyprutils::Memory::CSharedPointer<IRendererTexture>                   oldTex; // while loading a new one
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CImageResource> resource;
        Hyprutils::Math::Vector2D                                             size;

        bool                                                                  waitingForTex = false, failed = false;

        std::string                                                           lastPath = "";

        Hyprutils::Math::Vector2D                                             preferredSvgSize();
    };
}
