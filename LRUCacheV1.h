// LRUCacheV1.h : contains a definition of LRUCacheV1 class.
//   The LRUCacheV1 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV1 uses "hand made" double linked list over 
//   std::vector. An associative container to use for efficient key lookup 
//   is selected depending on the following template parameters:
//     MapOption -- an associative container to use for efficient key lookup.
//                  It can be one of the following: Options::StdMap, 
//                  Options::BoostMap, Options::StdUnorderedMap,
//                  Options::BoostUnorderedMap, Options::EmilibHashMap  
//
// Written by Sergey Korytnik 
//
#pragma once
#include <vector>
#include <cassert>
#include "LRUCacheOptions.h"
#include "LRUCacheMapOptions.h"

namespace LRUCache {
template <typename KeyType, typename ValueType, 
    typename MapOption = Options::StdMap<> 
>
class LRUCacheV1 {
private:
    using MapType = typename MapOption::template type<KeyType,size_t>;
public:
    LRUCacheV1() = delete;
    LRUCacheV1(const LRUCacheV1&) = delete;
    LRUCacheV1& operator=(const LRUCacheV1&) = delete;
    LRUCacheV1(LRUCacheV1&&) = default;
    LRUCacheV1& operator=(LRUCacheV1&&) = default;
    ~LRUCacheV1() = default;

    LRUCacheV1(size_t cacheSize) : maxCacheSize(cacheSize),
        keyMap(2 * cacheSize,ao)
    {
        entries.reserve(1 + cacheSize);
        // add the sentinel
        entries.emplace_back(0, 0, 
            ValueType(), typename MapType::iterator());
    }

    const ValueType* get(const KeyType& key) {
        assert(keyMap.size() <= maxCacheSize);
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushIntoQueue(l->second);
            return &entries[l->second].value;
        }
        return nullptr;
    } 

    bool put(const KeyType& key, const ValueType& value) {
        return finishPutOperation(
            keyMap.try_emplace(key, keyMap.size() + 1),
            std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return finishPutOperation(
            keyMap.try_emplace(key, keyMap.size() + 1),
            std::move(value));
    }

    bool put(KeyType&& key,const ValueType& value) {
        return finishPutOperation(
            keyMap.try_emplace(std::move(key), keyMap.size() + 1),
            std::move(ValueType(value)));
    }

    bool put(KeyType&& key, ValueType&& value) {
        return finishPutOperation(
            keyMap.try_emplace(std::move(key), keyMap.size() + 1),
            std::move(value));
    }

    static std::string description() {
        std::string s = "LRUCacheV1(";
        s += MapOption::description();
        s += " + custom list over vector)";
        return s;
    }
private:

    bool finishPutOperation(
        std::pair<typename MapType::iterator,bool> l, ValueType&& value) {
        size_t entryIndex = l.first->second;
        if (l.second == false) { // the key already exist in the map
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
            value(std::move(a_value)),keyLocation(std::move(a_keyLocation))
        {}
        size_t next;
        size_t prev;
        ValueType value;
        typename MapType::iterator keyLocation;
    };
    typename MapOption::MyAllocatorOption ao;
    // the zeroth entry is a sentinel in the LRU list
    // so the maximum number of entries is maxCacheSize + 1
    std::vector<Entry> entries;     
    MapType keyMap;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV1
