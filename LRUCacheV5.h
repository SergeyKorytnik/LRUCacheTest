// LRUCacheV5.h : contains a definition of LRUCacheV5 class.
//   The LRUCacheV5 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV5 uses hand made double linked list with
//   emilib::HashMap.
//
// Written by Sergey Korytnik 
#pragma once
#include "hash_map.hpp"
#include "LRUCacheOptions.h"
#include <vector>
#include <cassert>
namespace LRUCache {

template <typename KeyType, typename ValueType,
    typename HashOption = Options::StdHash 
>
class LRUCacheV5 {
private:
    struct Entry;
    using MapType = emilib::HashMap<KeyType, size_t, 
        typename HashOption::template type<KeyType> >;
public:
    LRUCacheV5() = delete;
    LRUCacheV5(const LRUCacheV5&) = delete;
    LRUCacheV5& operator=(const LRUCacheV5&) = delete;
    LRUCacheV5(LRUCacheV5&&) = default;
    LRUCacheV5& operator=(LRUCacheV5&&) = default;
    ~LRUCacheV5() = default;

    LRUCacheV5(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        // add the sentinel
        entries.emplace_back(0, 0, ValueType(),typename MapType::iterator());
        keyMap.reserve(2 * cacheSize);
    }

    static std::string description() {
        return std::string("LRUCacheV5(emilib::HashMap(") 
            + HashOption::description() 
            + "), custom double linked list over vector)";
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
        return finishPutOperation(
            keyMap.try_emplace(key, entryIndex),
            std::move(value));
    }

    bool put(KeyType&& key, ValueType value) {
        assert(keyMap.size() <= maxCacheSize);
        size_t entryIndex = keyMap.size() + 1; // +1 since the first entry is a sentinel
        return finishPutOperation(
            keyMap.try_emplace(std::move(key), entryIndex),
            std::move(value));
    }

private:
    bool finishPutOperation(
        std::pair<typename MapType::iterator, bool> l,
        ValueType&& value) {
        size_t entryIndex = l.first->second;
        if (l.second == false) { // the key already exist in the map
            entries[entryIndex].value = std::move(value);
            pushIntoQueue(entryIndex);
            return false;
        }

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
