// LRUCacheMapOptions.h : contains *Option classes configuring associative 
// containers that can be used as template parameters for LRUCacheV* 
// classes that are implementations of 
// a cache class with LRU replacement policy.
// The following options are defined and implemented here:
//   StdMap -- an option to use std::map with an allocator
//   BoostMap -- an option to use boost::container::map with an allocator
//   StdUnorderedMap -- an option to use std::unordered_map with a 
//   hash function and allocator.
//   BoostUnorderedMap -- an option to use boost::unordered_map with a 
//   hash function and allocator.
//   EmilibHashMap  -- an option to use emilib::HashMap  with a 
//   hash function.
//
// Written by Sergey Korytnik 
#pragma once

#include "LRUCacheOptions.h"
#include <boost/unordered_map.hpp>
#include <boost/container/map.hpp>
#include <unordered_map>
#include <map>
#include "hash_map.hpp"


namespace LRUCache {
namespace Options {

template <typename AllocatorOption = StdAllocator>
struct StdMap {
    template <typename KeyType, typename ValueType>
    struct OrderedMapType
    : public std::map<KeyType, ValueType,
            std::less<KeyType>,
            typename AllocatorOption::template type<
                std::pair<const KeyType, ValueType>
            >
        > {
        // to unify API with std::unordered_map
        OrderedMapType(size_t) {}
    };

    template <typename KeyType, typename ValueType>
    using type = OrderedMapType<KeyType, ValueType>;

    static std::string description() { 
        std::string a = AllocatorOption::description();
        if (a.empty()) {
            return "std::map";
        }
        else {
            return "std::map(" + a + ")";
        }
    }
};

template <typename AllocatorOption = StdAllocator>
struct BoostMap {
    template <typename KeyType, typename ValueType>
    struct OrderedMapType
        : public boost::container::map<KeyType, ValueType,
            std::less<KeyType>,
            typename AllocatorOption::template type<
                std::pair<const KeyType, ValueType>
            >
        > {
        // to unify API with std::unordered_map
        OrderedMapType(size_t) {}
    };

    template <typename KeyType, typename ValueType>
    using type = OrderedMapType<KeyType, ValueType>;

    static std::string description() {
        std::string a = AllocatorOption::description();
        if (a.empty()) {
            return "boost::container::map";
        }
        else {
            return "boost::container::map(" + a + ")";
        }
    }
};

template <typename HashOption = StdHash,
    typename AllocatorOption = StdAllocator>
struct StdUnorderedMap {

    template <typename KeyType, typename ValueType>
    using type = std::unordered_map<KeyType, ValueType,
        typename HashOption::template type<KeyType>,
            std::equal_to<KeyType>,
            typename AllocatorOption::template type<
                std::pair<const KeyType, ValueType>
            >
        >;

    static std::string description() {
        std::string a = AllocatorOption::description();
        std::string h = HashOption::description();
        if (a.empty()) {
            return "std::unordered_map(" + h + ")";
        }
        else {
            return "std::unordered_map(" + h + "," + a + ")";
        }
    }
};

template <typename HashOption = StdHash,
    typename AllocatorOption = StdAllocator>
struct BoostUnorderedMap {
    
    template <typename KeyType, typename ValueType>
    struct UnorderedMapType
        : public boost::unordered_map<KeyType, ValueType,
            typename HashOption::template type<KeyType>,
                std::equal_to<KeyType>,
                typename AllocatorOption::template type<
                    std::pair<const KeyType, ValueType
                >
            >
        > {
        using MyBase = boost::unordered_map < KeyType, ValueType,
            typename HashOption::template type<KeyType>,
            std::equal_to<KeyType>,
            typename AllocatorOption::template type<
                std::pair<const KeyType, ValueType>
            >
        >;
        UnorderedMapType(size_t numBuckets) : MyBase(numBuckets)
        {}

        // to unify API with std::unordered_map
        // in boost::unordered_map (1.62) emplace actually works as try_emplace
        // so we can just forward call to emplace
        template<class... OtherArgs>
        std::pair<typename MyBase::iterator,bool> try_emplace(const KeyType& key,
            OtherArgs&&... valueArgs)
        {	
            return MyBase::emplace(key, std::forward<OtherArgs>(valueArgs)...);
        }

        template<class... OtherArgs>
        std::pair<typename MyBase::iterator, bool> try_emplace(KeyType&& key,
            OtherArgs&&... valueArgs)
        {
            return MyBase::emplace(key, std::forward<OtherArgs>(valueArgs)...);
        }
    };

    template <typename KeyType, typename ValueType>
    using type = UnorderedMapType<KeyType, ValueType>;

    static std::string description() {
        std::string a = AllocatorOption::description();
        std::string h = HashOption::description();
        if (a.empty()) {
            return "boost::unordered_map(" + h + ")";
        }
        else {
            return "boost::unordered_map(" + h + "," + a + ")";
        }
    }
};

template <typename HashOption = StdHash>
    struct EmilibHashMap {

    template <typename KeyType, typename ValueType>
    using type = emilib::HashMap<KeyType, ValueType,
        typename HashOption::template type<KeyType>
    >;

    static std::string description() {
        return "emilib::HashMap(" + HashOption::description() + ")";
    }
};

} // namespace Options {
} // namespace LRUCache {

