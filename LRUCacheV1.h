// LRUCacheV1.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses "hand made" double linked list over std::vector
//   together with std::unordered_map if USE_UNORDERED_MAP is defined or with std::map otherwise.
//
// Written by Sergey Korytnik 
//
#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <map>
#include <cassert>
#include <type_traits>
#include <boost/pool/pool_alloc.hpp>


namespace LRUCacheV1 {
template <typename KeyType, typename ValueType, 
    // std::map will be used if Hasher is void!
    typename Hasher = std::hash<KeyType>, 
    bool use_fast_allocator = false
>
class LRUCache {
    struct Entry;
    static constexpr bool use_unordered_map = 
       !std::is_void<typename Hasher>::value;

    using MapPairAllocatorType = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<std::pair<const KeyType, size_t>>,
        std::allocator<std::pair<const KeyType, size_t>>
    >::type;
    using BaseOrderedMapType = std::map<KeyType, size_t,
        std::less<KeyType>,
        MapPairAllocatorType
    >;

    struct OrderedMapType : public BaseOrderedMapType {
        // to unify API with std::unordered_map<KeyType, size_t>
        OrderedMapType(size_t) {}
    };

    using UnorderedMapType = std::unordered_map<KeyType, size_t,
        typename Hasher,
        std::equal_to<KeyType>,
        MapPairAllocatorType
    >;

    using MapType = typename std::conditional<use_unordered_map,
        UnorderedMapType, OrderedMapType>::type;
public:
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;

    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)
        , keys(2 * cacheSize)
    {
        // add the sentinel
        entries.emplace_back(0, 0, 
            ValueType(), typename MapType::iterator());
    }

    static constexpr const char* description() {
        return use_unordered_map ?
            (use_fast_allocator ?
                "LRUCache(std::unordered_map"
                " + custom double linked list over vector + boost::fast_pool_allocator)"
                :
                "LRUCache(std::unordered_map"
                " + custom double linked list over vector + std::allocator)"
                )
            :
            (use_fast_allocator ?
                "LRUCache(std::map"
                " + custom double linked list over vector + boost::fast_pool_allocator)"
                :
                "LRUCache(std::map"
                " + custom double linked list over vector + std::allocator)"
                )
            ;
    }

    const ValueType* get(const KeyType& key) {
        assert(keys.size() <= maxCacheSize);
        auto l = keys.find(key);
        if (l != keys.end()) {
            pushIntoQueue(l->second);
            return &entries[l->second].value;
        }
        return nullptr;
    } 

    bool put(const KeyType& key, const ValueType& value) {
        assert(keys.size() <= maxCacheSize);
        // keys.size() + 1 since the first entry is a sentinel
        return finishPutOperation(
            keys.try_emplace(key, keys.size() + 1), std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        assert(keys.size() <= maxCacheSize);
        // keys.size() + 1 since the first entry is a sentinel
        return finishPutOperation(
            keys.try_emplace(key, keys.size() + 1), std::move(value));
    }

    bool put(KeyType&& key,const ValueType& value) {
        assert(keys.size() <= maxCacheSize);
        // keys.size() + 1 since the first entry is a sentinel
        return finishPutOperation(
            keys.try_emplace(std::move(key), keys.size() + 1), 
            std::move(ValueType(value)));
    }

    bool put(KeyType&& key, ValueType&& value) {
        assert(keys.size() <= maxCacheSize);
        // keys.size() + 1 since the first entry is a sentinel
        return finishPutOperation(
            keys.try_emplace(std::move(key), keys.size() + 1),std::move(value));
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

        assert(entryIndex == keys.size());
        if (entryIndex <= maxCacheSize) {
            entries.emplace_back(entryIndex,
                entryIndex,
                std::move(value), std::move(l.first));
        }
        else {
            entryIndex = entries[0].next;
            l.first->second = entryIndex;
            auto& e = entries[entryIndex];
            keys.erase(e.keyLocation);
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
    // the zeroth entry is a sentinel in the LRU list
    // so the maximum number of entries is maxCacheSize + 1
    std::vector<Entry> entries;     
    MapType keys;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV1
