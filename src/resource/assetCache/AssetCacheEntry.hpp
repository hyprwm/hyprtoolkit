#pragma once

#include <string_view>

#include <hyprutils/signal/Signal.hpp>

#include "../../helpers/Memory.hpp"

namespace Hyprtoolkit {
    class IRendererTexture;
}

namespace Hyprtoolkit::Asset {
    enum eAssetCacheEntryStatus : uint8_t {
        CACHE_ENTRY_PENDING = 0,
        CACHE_ENTRY_DONE    = 1,
    };

    class CAssetCacheEntry {
      public:
        CAssetCacheEntry(const std::string_view& source, const SP<IRendererTexture> tex);
        CAssetCacheEntry(const std::string_view& source);
        ~CAssetCacheEntry() = default;

        CAssetCacheEntry(const CAssetCacheEntry&) = delete;
        CAssetCacheEntry(CAssetCacheEntry&)       = delete;
        CAssetCacheEntry(CAssetCacheEntry&&)      = delete;

        std::string_view       source() const;
        SP<IRendererTexture>   tex() const;
        eAssetCacheEntryStatus status() const;

        // if created without a tex, this will mark asset as done
        void texDone(SP<IRendererTexture> tex);

        bool operator==(const CAssetCacheEntry& e) const {
            return m_tex == e.m_tex;
        }

        struct {
            Hyprutils::Signal::CSignalT<> done;
        } m_events;

      private:
        const std::string      m_source;
        SP<IRendererTexture>   m_tex;
        eAssetCacheEntryStatus m_status = CACHE_ENTRY_PENDING;
    };
};