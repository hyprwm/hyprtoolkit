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
        std::vector<uint8_t>       data;
        eImageFitMode              fitMode = IMAGE_FIT_MODE_STRETCH;
        CDynamicSize               size{CDynamicSize::HT_SIZE_PERCENT, CDynamicSize::HT_SIZE_PERCENT, {1, 1}};

        void                       setPath(std::string&& value);
        void                       setIcon(const SP<ISystemIconDescription>& value);
        void                       setData(std::vector<uint8_t>&& value);
    };

    struct SImageLoadRequest {
        uint64_t                                                              generation = 0;
        Hyprutils::Memory::CAtomicSharedPointer<Hyprgraphics::CImageResource> resource;
        SP<Asset::CAssetCacheEntry>                                           cacheEntry;
        eImageFitMode                                                         fitMode = IMAGE_FIT_MODE_STRETCH;
        std::string                                                           path;
    };

    struct SImageImpl {
        SImageData                                                 data;

        WP<CImageElement>                                          self;

        float                                                      lastScale = 1.F;

        Hyprutils::Memory::CSharedPointer<Asset::CAssetCacheEntry> cacheEntry;
        Hyprutils::Memory::CSharedPointer<Asset::CAssetCacheEntry> oldCacheEntry; // while loading a new one
        Hyprutils::Math::Vector2D                                  size;

        bool                                                       waitingForTex = false, failed = false;
        uint64_t                                                   requestGeneration = 0;
        std::vector<SP<SImageLoadRequest>>                         requests;

        std::string                                                lastPath = "";
        void*                                                      lastData = nullptr;

        Hyprutils::Math::Vector2D                                  preferredSvgSize();
        void                                                       postImageLoad(const SP<SImageLoadRequest>& request);
        void                                                       postImageScheduleRecalc();
        std::string                                                getCacheString();

        struct {
            Hyprutils::Signal::CHyprSignalListener cacheEntryDone;
        } listeners;
    };
}
