// LRUCacheV1.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses "hand made" double linked list over 
//   std::vector. An associative container to use for efficient key lookup 
//   is selected depending on the following template parameters:
//     -- if a hash function is provided (Hasher is not void) then 
//        a hash table is used (if use_boost_container then 
//        boost::unordered_map and std::unordered_map otherwise.
//     -- if no hash function is provided (Hasher is void) then an ordered 
//        lookup container is used (if use_boost_conatiner then
//        boost::container::map and std::map otherwise
//     -- if use_fast_allocator then boost::fast_pool_allocator will be used
//        in associative container instead of std::allocator    
//   
//
// Written by Sergey Korytnik 
//
#pragma once
#include <functional>
#include <vector>
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <unordered_map>
#include <map>
#include <cassert>
#include <type_traits>

namespace LRUCacheV1 {
template <typename KeyType, typename ValueType, 
    // std::map/boost::container::map will be used if Hasher is void!
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
    using MapPairAllocatorType = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<std::pair<const KeyType, size_t>>,
        std::allocator<std::pair<const KeyType, size_t>>
    >::type;

    using BaseOrderedMapType = typename std::conditional<use_boost_containers,
        boost::container::map<KeyType, size_t,
            std::less<KeyType>,
            MapPairAllocatorType
        >,
        std::map<KeyType, size_t,
            std::less<KeyType>,
            MapPairAllocatorType
        >
    >::type;

    struct OrderedMapType : public BaseOrderedMapType {
        // to unify API with std::unordered_map<KeyType, size_t>
        OrderedMapType(size_t) {}
    };

    using UnorderedMapType = typename std::conditional<use_boost_containers,
        boost::unordered_map<KeyType, size_t,
            Hasher,
            std::equal_to<KeyType>,
            MapPairAllocatorType
        >,
        std::unordered_map<KeyType, size_t,
            Hasher,
            std::equal_to<KeyType>,
            MapPairAllocatorType
        >
    >::type;

    using MapType = typename std::conditional<use_unordered_map,
        UnorderedMapType, OrderedMapType>::type;
public:
    using value_type = ValueType;
    using key_type = KeyType;
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;

    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)
        , keyMap(2 * cacheSize)
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
            insertIntoMap(key), 
            std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return finishPutOperation(
            insertIntoMap(key),
            std::move(value));
    }

    bool put(KeyType&& key,const ValueType& value) {
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
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map + std::hash"
                            " + custom list over vector"
                            ")";
                    }
                }
                else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(boost::unordered_map + boost::hash"
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map + boost::hash"
                            " + custom list over vector"
                            ")";
                    }
                }
                else {
                    if (use_fast_allocator) {
                        return "LRUCache(boost::unordered_map"
                            " + unknown hash function"
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(boost::unordered_map"
                            " + unknown hash function"
                            " + custom list over vector"
                            ")";
                    }
                }
            }
            else {
                if (std::is_same < Hasher, std::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + std::hash"
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + std::hash"
                            " + custom list over vector"
                            ")";
                    }
                }
                else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + boost::hash"
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + boost::hash"
                            " + custom list over vector"
                            ")";
                    }
                }
                else {
                    if (use_fast_allocator) {
                        return "LRUCache(std::unordered_map + unknown hash function"
                            " + custom list over vector"
                            " + boost::fast_pool_allocator)";
                    }
                    else {
                        return "LRUCache(std::unordered_map + unknown hash function"
                            " + custom list over vector"
                            ")";
                    }
                }
            }
        }
        else {
            if (use_boost_containers) {
                if (use_fast_allocator) {
                    return "LRUCache(boost::container::map"
                        " + custom list over vector"
                        " + boost::fast_pool_allocator)";
                }
                else {
                    return "LRUCache(boost::container::map"
                        " + custom list over vector"
                        ")";
                }
            }
            else {
                if (use_fast_allocator) {
                    return "LRUCache(std::map"
                        " + custom list over vector"
                        " + boost::fast_pool_allocator)";
                }
                else {
                    return "LRUCache(std::map"
                        " + custom list over vector"
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
        assert(keyMap.size() <= maxCacheSize);
        // keyMap.size() + 1 since the first entry is a sentinel
        return keyMap.emplace(key, keyMap.size() + 1);
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<!ete>* = nullptr
    >
    auto insertIntoMap(const KeyType& key)
    {
        assert(keyMap.size() <= maxCacheSize);
        // keyMap.size() + 1 since the first entry is a sentinel
        return keyMap.try_emplace(key, keyMap.size() + 1);
    }
    
    template<bool ete = emulate_try_emplace, 
        std::enable_if_t<ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key )
    {
        assert(keyMap.size() <= maxCacheSize);
        // keyMap.size() + 1 since the first entry is a sentinel
        return keyMap.emplace(std::move(key), keyMap.size() + 1);
    }

    template<bool ete = emulate_try_emplace,
        std::enable_if_t<!ete>* = nullptr
    >
    auto insertIntoMap(KeyType&& key)
    {
        assert(keyMap.size() <= maxCacheSize);
        // keyMap.size() + 1 since the first entry is a sentinel
        return keyMap.try_emplace(std::move(key), keyMap.size() + 1);
    }

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
    // the zeroth entry is a sentinel in the LRU list
    // so the maximum number of entries is maxCacheSize + 1
    std::vector<Entry> entries;     
    MapType keyMap;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV1
