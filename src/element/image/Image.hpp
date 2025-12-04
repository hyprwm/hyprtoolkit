#include <hyprtoolkit/element/Image.hpp>

#include "../../helpers/Memory.hpp"
#include "../../resource/assetCache/AssetCacheEntry.hpp"

namespace Hyprtoolkit {
    struct SImageData {
        std::string                path;
        float                      a        = 1.F;
        int                        rounding = 0;
        bool                       sync     = false;
        SP<ISystemIconDescription> icon;
        eImageFitMode              fitMode = IMAGE_FIT_MODE_STRETCH;
        CDynamicSize               size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};
    };

    struct SImageImpl {
        SImageData                                                            data;

        WP<CImageElement>                                                     self;

        float                                                                 lastScale = 1.F;

        Hyprutils::Memory::CSharedPointer<Asset::CAssetCacheEntry>            cacheEntry;
        Hyprutils::Memory::CSharedPointer<Asset::CAssetCacheEntry>            oldCacheEntry; // while loading a new one
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CImageResource> resource;
        Hyprutils::Math::Vector2D                                             size;

        bool                                                                  waitingForTex = false, failed = false;

        std::string                                                           lastPath = "";

        Hyprutils::Math::Vector2D                                             preferredSvgSize();
        void                                                                  postImageLoad();
        void                                                                  postImageScheduleRecalc();
        std::string                                                           getCacheString();

        struct {
            Hyprutils::Signal::CHyprSignalListener cacheEntryDone;
        } listeners;
    };
}
