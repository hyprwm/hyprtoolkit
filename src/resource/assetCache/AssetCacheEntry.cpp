#include "AssetCacheEntry.hpp"

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Asset;

CAssetCacheEntry::CAssetCacheEntry(const std::string_view& source, const SP<IRendererTexture> tex) : m_source(source), m_tex(tex), m_status(CACHE_ENTRY_DONE) {
    ;
}

CAssetCacheEntry::CAssetCacheEntry(const std::string_view& source) : m_source(source) {
    ;
}

std::string_view CAssetCacheEntry::source() const {
    return m_source;
}

SP<IRendererTexture> CAssetCacheEntry::tex() const {
    return m_tex;
}

eAssetCacheEntryStatus CAssetCacheEntry::status() const {
    return m_status;
}

void CAssetCacheEntry::texDone(SP<IRendererTexture> tex) {
    m_tex    = tex;
    m_status = CACHE_ENTRY_DONE;
    m_events.done.emit();
}
