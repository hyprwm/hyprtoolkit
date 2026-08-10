#include "AssetCacheEntry.hpp"
#include "../../core/BackendContext.hpp"

using namespace Hyprtoolkit;
using namespace Hyprtoolkit::Asset;

CAssetCacheEntry::CAssetCacheEntry(const std::string_view& source, const SP<IRendererTexture> tex) :
    m_source(source), m_tex(tex), m_status(CACHE_ENTRY_DONE), m_generation(g_backendServices ? g_backendServices->lifetime->generation : 0) {
    ;
}

CAssetCacheEntry::CAssetCacheEntry(const std::string_view& source) : m_source(source), m_generation(g_backendServices ? g_backendServices->lifetime->generation : 0) {
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

uint64_t CAssetCacheEntry::generation() const {
    return m_generation;
}

void CAssetCacheEntry::texDone(SP<IRendererTexture> tex) {
    m_tex    = tex;
    m_status = CACHE_ENTRY_DONE;
    m_events.done.emit();
}
