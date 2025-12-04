#pragma once

#include "AssetCacheEntry.hpp"

#include <vector>

namespace Hyprtoolkit::Asset {

    // AssetCache is a cache for loading assets. Since we keep assets (images) as
    // textures in VRAM, we don't want to load the same raster asset multiple times.
    // IMPORTANT: AssetCache holds only WEAK references to the entries. You need
    // to hold your own ref somewhere to avoid the asset going dead
    class CAssetCache {
      public:
        CAssetCache()  = default;
        ~CAssetCache() = default;

        CAssetCache(const CAssetCache&) = delete;
        CAssetCache(CAssetCache&)       = delete;
        CAssetCache(CAssetCache&&)      = delete;

        // TODO: implement this for svg images. These can be loaded at different res's
        SP<CAssetCacheEntry> get(const std::string_view& source);
        void                 cache(SP<CAssetCacheEntry> entry);

      private:
        void                              gc();

        std::vector<WP<CAssetCacheEntry>> m_entries;
    };

    SP<CAssetCache> assetCache();
};