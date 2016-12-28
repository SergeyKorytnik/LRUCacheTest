// LRUCacheOptions.h : contains the most common *Option classes 
// -- template parameters for LRUCacheV* classes that are implementations of 
// a cache class with LRU replacement policy.
// The following options are defined and implemented here:
//   StdAllocator -- an option to use std::allocator with containers.
//   FastPoolAllocator -- an option to use boost::fast_pool_allocator with
//   containers.
//   StdHash -- an option to use std::hash with hash-table based containers
//   BoostHash -- an option to use boost::hash with hash-table based containers
//   VoidHash -- an option to use an ordered index instead of hash-table based 
//               containers. It is applicable to LRUCache::LRUCacheV3 and
//               LRUCache::LRUCacheV4
//
// Written by Sergey Korytnik 
#pragma once

#include <functional>
#include <string>
#include <boost/pool/pool_alloc.hpp>
#include <boost/functional/hash.hpp>

namespace LRUCache {
namespace Options {

    struct StdAllocator {
        template <typename T>
        using type = std::allocator<T>;

        static std::string description() { return ""; }
    };

    struct FastPoolAllocator {
        template <typename T>
        using type = boost::fast_pool_allocator<T>;

        static std::string description() { return "boost::fast_pool_allocator"; }
    };

    struct StdHash {
        template <typename T>
        using type = std::hash<T>;

        static std::string description() { return "std::hash"; }
    };

    struct BoostHash {
        template <typename T>
        using type = boost::hash<T>;

        static std::string description() { return "boost::hash"; }
    };

    struct VoidHash {
        template <typename T>
        using type = void;

        static std::string description() { return "void"; }
    };

} // namespace Options {
} // namespace LRUCache {