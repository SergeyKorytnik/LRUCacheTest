// LRUCacheV2.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses std::list together with an associative
//   container for efficient key lookup. The associative container
//   is selected depending on the following template parameters:
//     -- if a hash function is provided (Hasher is not void) then 
//        a hash table is used (if use_boost_container then 
//        boost::unordered_map and std::unordered_map otherwise.
//     -- if no hash function is provided (Hasher is void) then an ordered 
//        lookup container is used (if use_boost_conatiner then
//        boost::container::map and std::map otherwise
//     -- if use_fast_allocator then boost::fast_pool_allocator will be used
//        in associative container instead of std::allocator.
//   Also if use_boost_containers is true then std::list is replaced by 
//   boost::container::list.
//
// Written by Sergey Korytnik 
//
// Note:
//    Special thanks to Fedor Chelnokov 
//    for suggestion to use only a map and a list; 
//    And also for reminding me about existence of list::splice operation. :-)
//
#pragma once
#include <functional>
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <boost/container/list.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <list>
#include <unordered_map>
#include <map>
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
private:
    static constexpr bool use_unordered_map =
        !std::is_void<Hasher>::value;
    static constexpr bool emulate_try_emplace =
        use_boost_containers && use_unordered_map;

    struct Entry;
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
    using value_type = ValueType;
    using key_type = KeyType;
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;
    LRUCache(size_t cacheSize)
        : maxCacheSize(cacheSize), keyMap(2*cacheSize) {}

    const ValueType* get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(l->second.queueLocation);
            return &l->second.queueLocation->value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, const ValueType& value) {
        return finishPutOperation(
            insertIntoMap(key),
            std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return finishPutOperation(
            insertIntoMap(key),
            std::move(value));
    }

    bool put(KeyType&& key, const ValueType& value) {
        return finishPutOperation(
            insertIntoMap(std::move(key)),
            std::move(ValueType(value)));
    }

    bool put(KeyType&& key, ValueType&& value) {
        return finishPutOperation(
            insertIntoMap(std::move(key)),
            std::move(value));
    }

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

private:
    template<bool ete = emulate_try_emplace,
        std::enable_if_t<ete>* = nullptr
    >
    auto insertIntoMap(const KeyType& key)
    {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            return std::pair<typename MapType::iterator, bool>(l, false);
        }
        return keyMap.emplace(key, Entry());
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<!ete>* = nullptr
    >
        auto insertIntoMap(const KeyType& key)
    {
        return keyMap.try_emplace(key, Entry());
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            return std::pair<typename MapType::iterator,bool>(l,false);
        }
        return keyMap.emplace(std::move(key), Entry());
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<!ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key) {
        return keyMap.try_emplace(std::move(key), Entry());
    }

    bool finishPutOperation(
        std::pair<typename MapType::iterator, bool> l, ValueType&& value) {
        if (l.second == false) { // the key already exist in the map
            l.first->second.queueLocation->value = std::move(value);
            pushToQueueEnd(l.first->second.queueLocation);
            return false;
        }
        if (keyMap.size() > maxCacheSize) {
            auto eloc = lruQueue.front().mapLocation;
            lruQueue.pop_front();
            keyMap.erase(eloc);
        }
        lruQueue.emplace_back(std::move(value), l.first);
        l.first->second.queueLocation = --lruQueue.end();
        return true;
    }

    void pushToQueueEnd(typename QueueType::iterator it) {
        lruQueue.splice(lruQueue.end(), lruQueue, it);
    }

    struct Entry {
        //explicit Entry(const ValueType& aValue) : value(aValue) {}
        //explicit Entry(ValueType&& aValue) : value(std::move(aValue)) {}
        //ValueType value;
        typename QueueType::iterator queueLocation;
    };

    struct QueueItem {
        QueueItem(ValueType&& aValue,
            const typename MapType::iterator& it) 
        : value(std::move(aValue)), mapLocation(it) {}
        ValueType value;
        typename MapType::iterator mapLocation;
    };
    MapType keyMap;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV2
