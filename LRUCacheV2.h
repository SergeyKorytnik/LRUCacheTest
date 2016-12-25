// LRUCacheV2.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses std::list together with std::unordered_map 
//   or with std::map depending on whether a hash function is provided 
//   as a template parameter.
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
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <boost/container/list.hpp>
#include <list>
#include <unordered_map>
#include <map>
#include <boost/pool/pool_alloc.hpp>
#include <cassert>
#include <type_traits>


namespace LRUCacheV2 {

template <typename KeyType, typename ValueType, 
    // std::map will be used if Hasher is void!
    class Hasher = std::hash<KeyType>,
    bool use_fast_allocator = false,
    bool use_boost_containers = false
>
class LRUCache {
public:
    using value_type = ValueType;
    using key_type = KeyType;
private:
    struct Entry;
    static constexpr bool use_unordered_map =
        !std::is_void<Hasher>::value;
    static constexpr bool emulate_try_emplace =
        use_boost_containers && use_unordered_map;

    using MapPairAllocatorType = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<std::pair<const KeyType, Entry>>,
        std::allocator<std::pair<const KeyType, Entry>>
    >::type;

    using BaseOrderedMapType = typename std::conditional<use_boost_containers,
        boost::container::map<KeyType, Entry,
        std::less<KeyType>,
        MapPairAllocatorType
        >,
        std::map<KeyType, Entry,
        std::less<KeyType>,
        MapPairAllocatorType
        >
    >::type;

    struct OrderedMapType : public BaseOrderedMapType {
        // to unify API with std::unordered_map<KeyType, size_t>
        OrderedMapType(size_t) {}
    };

    using UnorderedMapType = typename std::conditional<use_boost_containers,
        boost::unordered_map<KeyType, Entry,
        Hasher,
        std::equal_to<KeyType>,
        MapPairAllocatorType
        >,
        std::unordered_map<KeyType, Entry,
        Hasher,
        std::equal_to<KeyType>,
        MapPairAllocatorType
        >
    >::type;

    using MapType = typename std::conditional<use_unordered_map,
        UnorderedMapType, OrderedMapType>::type;

    // the QueueItem struct is introduced to make G++-6.2 happy. 
    // clang++ and VC++2015 can directly work with MapType::iterator.
    struct QueueItem; 
    using QueueItemAllocator = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<QueueItem>,
        std::allocator<QueueItem>
    >::type;
    using QueueType = typename std::conditional<use_boost_containers,
        boost::container::list<QueueItem, QueueItemAllocator>,
        std::list<QueueItem, QueueItemAllocator>
    >::type;
public:
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;
    LRUCache(size_t cacheSize)
        : maxCacheSize(cacheSize), keyMap(2*cacheSize) {}

    static const char* description() {
        if (use_unordered_map) {
            if (use_boost_containers) {
                if (std::is_same < Hasher, std::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(boost::unordered_map + std::hash"
                            " + boost::container::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map + std::hash"
                            " + boost::container::list"
                            ")";
                    }
                }
                else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(boost::unordered_map + boost::hash"
                            " + boost::container::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map + boost::hash"
                            " + boost::container::list"
                            ")";
                    }
                }
                else {
                    if (use_fast_allocator) {
                        return "LRUCache(boost::unordered_map + unknown hash function"
                            " + boost::container::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map + unknown hash function"
                            " + boost::container::list"
                            ")";
                    }
                }
            }
            else {
                if (std::is_same < Hasher, std::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + std::hash"
                            " + std::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + std::hash"
                            " + std::list"
                            ")";
                    }
                }
                else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + boost::hash"
                            " + std::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + boost::hash"
                            " + std::list"
                            ")";
                    }
                }
                else {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + unknown hash function"
                            " + std::list"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + unknown hash function"
                            " + std::list"
                            ")";
                    }
                }
            }
        }
        else {
            if (use_boost_containers) {
                if (use_fast_allocator) {
                    return "LRUCache(boost::container::map"
                        " + boost::container::list"
                        " + boost::fast_pool_allocator)";
                }
                else {
                    return "LRUCache(boost::container::map"
                        " + boost::container::list"
                        ")";
                }
            }
            else {
                if (use_fast_allocator) {
                    return "LRUCache(std::map"
                        " + std::list"
                        " + boost::fast_pool_allocator)";
                }
                else {
                    return "LRUCache(std::map"
                        " + std::list"
                        ")";
                }
            }
        }
    }

    const ValueType* get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
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
    template<bool ete = emulate_try_emplace,
        std::enable_if_t<ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key, Entry&& e) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            return std::pair<typename MapType::iterator,bool>(l,false);
        }
        return keyMap.emplace(std::move(key), std::move(e));
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<!ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key, Entry&& e) {
        return keyMap.try_emplace(std::move(key), std::move(e));
    }

    bool put(KeyType&& key, Entry&& e) {
        auto l = insertIntoMap<use_boost_containers && use_unordered_map>(
            std::move(key), std::move(e));
        if (l.second == false) { // the key already exist in the map
            l.first->second.value = std::move(e.value);
            pushToQueueEnd(l.first->second.queueLocation);
            return false;
        }
        if (keyMap.size() > maxCacheSize) {
            auto eloc = lruQueue.front().mapLocation;
            lruQueue.pop_front();
            keyMap.erase(eloc);
        }
        lruQueue.emplace_back(l.first);
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

    struct QueueItem {
        QueueItem(const typename MapType::iterator& it) : mapLocation(it) {}
        typename MapType::iterator mapLocation;
    };
    MapType keyMap;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV2
