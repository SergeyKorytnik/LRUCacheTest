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
    // std::map will be used if Hasher is void!
    typename Hasher = std::hash<KeyType>,
    bool use_fast_allocator = false
>
class LRUCache {
    struct Entry;
    static constexpr bool use_unordered_map =
        !std::is_void<typename Hasher>::value;
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
        Hasher,
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
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;
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

    const ValueType* get(const KeyType& key) {
        auto l = entries.find(key);
        if (l != entries.end()) {
            pushToQueueEnd(l->second.queueLocation);
            return &l->second.value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, const ValueType& value) {
        return put(std::move(KeyType(key)), Entry(value));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return put(std::move(KeyType(key)), Entry(std::move(value)));
    }
    bool put(KeyType&& key, const ValueType& value) {
        return put(std::move(key), Entry(value));
    }
    bool put(KeyType&& key, ValueType&& value) {
        return put(std::move(key), Entry(std::move(value)));
    }

private:
    bool put(KeyType&& key, Entry&& e) {
        auto l = entries.try_emplace(std::move(key), std::move(e));
        if (l.second == false) { // the key already exist in the map
            l.first->second.value = std::move(e.value);
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
    void pushToQueueEnd(typename QueueType::iterator it) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            it);
    }

    struct Entry {
        Entry(const ValueType& aValue) : value(aValue) {}
        Entry(ValueType&& aValue) : value(std::move(aValue)) {}
        ValueType value;
        typename QueueType::iterator queueLocation;
    };
    MapType entries;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV2
