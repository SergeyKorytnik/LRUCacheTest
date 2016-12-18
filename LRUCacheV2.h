// LRUCacheV2.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses std::list together with std::unordered_map 
//   if USE_UNORDERED_MAP is defined or with std::map otherwise.
//
// Written by Sergey Korytnik 
//
// Note:
//    Special thanks to Fedor Chelnokov 
//    for suggestion to use only a map and a list. 
//    And also for reminding me about existence of list::splice operation.
//
#pragma once
#include <functional>
#include <list>
#include <unordered_map>
#include <map>
#include <boost/pool/pool_alloc.hpp>
#include <cassert>
#include <type_traits>


namespace LRUCacheV2 {

template <typename KeyType, typename ValueType, 
    bool use_unordered_map,
    bool use_fast_allocator
>
class LRUCache {
    struct Entry;
    using MapPairAllocatorType = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<std::pair<const KeyType, Entry>>,
        std::allocator<std::pair<const KeyType, Entry>>
    >::type;
    using BaseOrderedMapType = std::map<KeyType, Entry,
        std::less<KeyType>,
        MapPairAllocatorType
    >;
    struct OrderedMapType : public BaseOrderedMapType {
        // to unify API with std::unordered_map<KeyType, Entry>
        OrderedMapType(size_t) {}
    };
    using UnorderedMapType = std::unordered_map<KeyType, Entry,
        std::hash<KeyType>,
        std::equal_to<KeyType>,
        MapPairAllocatorType
    >;

    using MapType = typename std::conditional<use_unordered_map,
        UnorderedMapType, OrderedMapType>::type;

    using QueueItemAllocator = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<typename MapType::iterator>,
        std::allocator<typename MapType::iterator>
    >::type;

    using QueueType = std::list<typename MapType::iterator, QueueItemAllocator>;
public:
    LRUCache(size_t cacheSize) 
        : maxCacheSize(cacheSize), entries(2*cacheSize) {}

    static constexpr const char* description() {
        return use_unordered_map ? 
            (use_fast_allocator ?
                "LRUCache(std::unordered_map"
                " + std::list + boost::fast_pool_allocator)"
                :
                "LRUCache(std::unordered_map"
                " + std::list + std::allocator)"
            )
            :
            (use_fast_allocator ?
                "LRUCache(std::map"
                " + std::list + boost::fast_pool_allocator)"
                :
                "LRUCache(std::map"
                " + std::list + std::allocator)"
                )
            ;
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto l = entries.find(key);
        if (l != entries.end()) {
            pushToQueueEnd(l->second.queueLocation);
            return{ true, l->second.value };
        }
        return{ false, ValueType() };
    }

    bool put(const KeyType& key, const ValueType& value) {
        return put(std::move(KeyType(key)), std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return put(std::move(KeyType(key)), std::move(value));
    }
    bool put(KeyType&& key, const ValueType& value) {
        return put(std::move(key), std::move(ValueType(value)));
    }

    bool put(KeyType&& key, ValueType&& value) {
        auto l = entries.try_emplace(std::move(key), std::move(value));
        if (l.second == false) { // the key already exist in the map
            l.first->second.value = std::move(value);
            pushToQueueEnd(l.first->second.queueLocation);
            return false;
        }

        if (entries.size() > maxCacheSize) {
            auto eloc = lruQueue.front();
            lruQueue.pop_front();
            entries.erase(eloc);
        }

        lruQueue.push_back(l.first);
        l.first->second.queueLocation = --lruQueue.end();
        return true;
    }
private:
    void pushToQueueEnd(typename QueueType::iterator it) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            it);
    }

    struct Entry {
        Entry(ValueType&& aValue) : value(std::move(aValue)) {}
        ValueType value;
        typename QueueType::iterator queueLocation;
    };
    MapType entries;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV2
