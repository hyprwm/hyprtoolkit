#include "AssetCache.hpp"
#include "../../core/BackendContext.hpp"

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Asset;

SP<CAssetCache> Asset::assetCache() {
    static auto cache = makeShared<CAssetCache>();
    return cache;
}

SP<CAssetCacheEntry> CAssetCache::get(const std::string_view& source) {
    if (!g_backendServices)
        return nullptr;

    for (const auto& e : m_entries) {
        if (e && e->source() == source && e->generation() == g_backendServices->lifetime->generation)
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
