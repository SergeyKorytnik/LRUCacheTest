// LRUCacheV5.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses hand made double linked list with
//   emilib::HashMap.
//
// Written by Sergey Korytnik 
#pragma once
#include "hash_map.hpp"
#include <vector>
#include <cassert>

template <typename KeyType, typename ValueType>
class LRUCache {
    struct Entry;
    using MapType = emilib::HashMap<KeyType, size_t>;
public:
    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        // add the sentinel
        entries.emplace_back(0, 0, ValueType(), MapType::iterator());
        keys.reserve(cacheSize);
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        assert(keys.size() <= maxCacheSize);
        auto l = keys.find(key);
        if (l != keys.end()) {
            pushIntoQueue(l->second);
            return{ true, entries[l->second].value };
        }
        return{ false, ValueType() };
    }

    void put(const KeyType& key, const ValueType& value) {
        put(std::move(KeyType(key)), std::move(ValueType(value)));
    }

    void put(const KeyType& key, ValueType&& value
    ) {
        assert(keys.size() <= maxCacheSize);
        size_t entryIndex = keys.size() + 1; // +1 since the first entry is a sentinel
        auto l = keys.insert(key, entryIndex);
        if (l.second == false) { // the key already exist in the map
            entryIndex = l.first->second;
            entries[entryIndex].value = std::forward<ValueType>(value);
            pushIntoQueue(entryIndex);
            return;
        }

        assert(entryIndex == keys.size());
        if (entryIndex <= maxCacheSize) {
            entries.emplace_back(entryIndex,
                entryIndex,
                std::forward<ValueType>(value), std::move(l.first));
        }
        else {
            entryIndex = entries[0].next;
            l.first->second = entryIndex;
            auto& e = entries[entryIndex];
            keys.erase(e.keyLocation);
            e.value = std::forward<ValueType>(value);
            e.keyLocation = std::move(l.first);
        }
        pushIntoQueue(entryIndex);
    }

private:
    void pushIntoQueue(size_t entryIndex) {
        auto& e = entries[entryIndex];
        entries[e.prev].next = e.next;
        entries[e.next].prev = e.prev;

        auto& sentinel = entries.front();
        e.prev = sentinel.prev;
        e.next = 0;
        entries[sentinel.prev].next = entryIndex;
        sentinel.prev = entryIndex;
    }

    struct Entry {
        Entry(size_t a_next, size_t a_prev,
            ValueType&& a_value, typename MapType::iterator&& a_keyLocation
        ) : next(a_next), prev(a_prev),
            value(std::move(a_value)), keyLocation(std::move(a_keyLocation))
        {}
        size_t next;
        size_t prev;
        ValueType value;
        typename MapType::iterator keyLocation;
    };
    // the zeroth entry is a sentinel in the LRU list
    // so the maximum number of entries is maxCacheSize + 1
    std::vector<Entry> entries;
    MapType keys;
    const size_t maxCacheSize;
};
