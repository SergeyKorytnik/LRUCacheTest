// LRUCacheV5.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses hand made double linked list with
//   emilib::HashMap.
//
// Written by Sergey Korytnik 
#pragma once
#include "hash_map.hpp"
#include <functional>
#include <vector>
#include <cassert>
namespace LRUCacheV5 {

template <typename KeyType, typename ValueType,
    typename Hasher = std::hash<KeyType>
>
class LRUCache {
public:
    using value_type = ValueType;
    using key_type = KeyType;
private:
    struct Entry;
    using MapType = emilib::HashMap<KeyType, size_t, Hasher>;
public:
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;

    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        // add the sentinel
        entries.emplace_back(0, 0, ValueType(),typename MapType::iterator());
        keyMap.reserve(cacheSize);
    }

    static const char* description() {
        if (std::is_same < Hasher, std::hash<KeyType>>::value) {
            return "LRUCache(emilib::HashMap + std::hash"
                " + custom double linked list over vector)";
        }
        else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
            return "LRUCache(emilib::HashMap + boost::hash"
                " + custom double linked list over vector)";
        }
        else {
            return "LRUCache(emilib::HashMap + unknown hash function"
                " + custom double linked list over vector)";
        }
    }   

    const ValueType* get(const KeyType& key) {
        assert(keyMap.size() <= maxCacheSize);
        const size_t* l = keyMap.try_get(key);
        if (l != nullptr) {
            pushIntoQueue(*l);
            return &entries[*l].value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, ValueType value) {
        assert(keyMap.size() <= maxCacheSize);
        size_t entryIndex = keyMap.size() + 1; // +1 since the first entry is a sentinel
        auto l = keyMap.insert(key, entryIndex);
        if (l.second == false) { // the key already exist in the map
            entryIndex = l.first->second;
            entries[entryIndex].value = std::move(value);
            pushIntoQueue(entryIndex);
            return false;
        }

        assert(entryIndex == keyMap.size());
        if (entryIndex <= maxCacheSize) {
            entries.emplace_back(entryIndex,
                entryIndex,
                std::move(value), std::move(l.first));
        }
        else {
            entryIndex = entries[0].next;
            l.first->second = entryIndex;
            auto& e = entries[entryIndex];
            keyMap.erase(e.keyLocation);
            e.value = std::move(value);
            e.keyLocation = std::move(l.first);
        }
        pushIntoQueue(entryIndex);
        return true;
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
    MapType keyMap;
    const size_t maxCacheSize;
};

} // namespace LRUCacheV5
