#include "AssetCache.hpp"

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Asset;

SP<CAssetCache> Asset::assetCache() {
    static auto cache = makeShared<CAssetCache>();
    return cache;
}

SP<CAssetCacheEntry> CAssetCache::get(const std::string_view& source) {
    for (const auto& e : m_entries) {
        if (e && e->source() == source)
            return e.lock();
    }

    return nullptr;
}

void CAssetCache::cache(SP<CAssetCacheEntry> entry) {
    gc();
    m_entries.emplace_back(entry);
}

void CAssetCache::gc() {
    std::erase_if(m_entries, [](const auto& e) { return !e; });
}
