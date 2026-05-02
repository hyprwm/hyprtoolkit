#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Hyprtoolkit {

    // Open-addressing string-keyed map for insert + lookup workloads.
    // Linear probing, power-of-two capacity, load factor 0.5.
    // No erase support: this exists for caches that only grow during a
    // session (e.g. resolved icon paths). Lookups are a contiguous walk
    // from the home bucket, so hot paths stay in L1.
    template <typename V>
    class CFastStringMap {
      public:
        CFastStringMap() : m_buckets(INITIAL_CAP), m_mask(INITIAL_CAP - 1) {}

        void reserve(size_t n) {
            size_t want = INITIAL_CAP;
            while (want < n * 2)
                want <<= 1;
            if (want > m_buckets.size())
                rehash(want);
        }

        void set(std::string key, V value) {
            if ((m_count + 1) * 2 > m_buckets.size())
                rehash(m_buckets.size() << 1);

            const size_t h = hash(key);
            size_t       i = h & m_mask;
            while (m_buckets[i].used && (m_buckets[i].hashv != h || m_buckets[i].key != key)) {
                i = (i + 1) & m_mask;
            }

            if (!m_buckets[i].used) {
                m_buckets[i].used  = true;
                m_buckets[i].hashv = h;
                m_buckets[i].key   = std::move(key);
                ++m_count;
            }
            m_buckets[i].value = std::move(value);
        }

        const V* get(const std::string& key) const {
            const size_t h = hash(key);
            size_t       i = h & m_mask;
            while (m_buckets[i].used) {
                if (m_buckets[i].hashv == h && m_buckets[i].key == key)
                    return &m_buckets[i].value;
                i = (i + 1) & m_mask;
            }
            return nullptr;
        }

        bool   empty() const { return m_count == 0; }
        size_t size() const { return m_count; }

      private:
        static constexpr size_t INITIAL_CAP = 16;

        struct SBucket {
            bool        used  = false;
            size_t      hashv = 0;
            std::string key;
            V           value{};
        };

        std::vector<SBucket> m_buckets;
        size_t               m_count = 0;
        size_t               m_mask  = 0;

        static size_t hash(const std::string& s) {
            size_t h = 14695981039346656037ULL;
            for (unsigned char c : s) {
                h ^= c;
                h *= 1099511628211ULL;
            }
            return h;
        }

        void rehash(size_t newCap) {
            std::vector<SBucket> oldB(newCap);
            std::swap(oldB, m_buckets);
            m_mask  = newCap - 1;
            m_count = 0;
            for (auto& b : oldB) {
                if (!b.used)
                    continue;
                size_t i = b.hashv & m_mask;
                while (m_buckets[i].used)
                    i = (i + 1) & m_mask;
                m_buckets[i] = std::move(b);
                ++m_count;
            }
        }
    };

}
