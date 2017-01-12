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
#include <boost/container/pmr/unsynchronized_pool_resource.hpp>
#include <boost/container/pmr/polymorphic_allocator.hpp>
#include <boost/container/pmr/pool_options.hpp>

#if defined(__GNUC__) && !defined(__clang__)
// the g++6.2 provides hash functor only 
// for standard basic_string specializations
// the code below provides a workaround for a generic case
namespace std {
template<class _Elem,
    class _Traits,
    class _Alloc>
    struct hash<basic_string<_Elem, _Traits, _Alloc> >
{	// hash functor for basic_string
    typedef basic_string<_Elem, _Traits, _Alloc> argument_type;
    typedef size_t result_type;
    size_t operator()(const argument_type& s) const noexcept
    { 
        return std::_Hash_impl::hash(s.data(), s.length()); 
    }
};

template<class _Elem,
    class _Traits,
    class _Alloc>
struct __is_fast_hash<hash<basic_string<_Elem, _Traits, _Alloc> > > 
    : std::false_type
{ };
} // namespace std
#endif

namespace LRUCache {
namespace Options {

    struct StdAllocator {
        template <typename T>
        using type = std::allocator<T>;

        static std::string description() { return ""; }
        
        template <typename T>
        static type<T> getAllocator() { return type<T>(); }
    };

    struct FastPoolAllocator {
        template <typename T>
        using type = boost::fast_pool_allocator<T, 
            boost::default_user_allocator_new_delete, 
            boost::details::pool::null_mutex>;
            //boost::fast_pool_allocator<T>;

        static std::string description() { return "boost::fast_pool_allocator<null_mutex>"; }
        template <typename T>
        static type<T> getAllocator() { return type<T>(); }
    };

    struct UnsynchronizedMemoryResource {
        template <typename T>
        using type = boost::container::pmr::polymorphic_allocator<T>;

        static std::string description() { 
            return "boost::container::pmr::unsynchronized_pool_resource"; 
        }
        template <typename T>
        type<T> getAllocator() noexcept {
            return type<T>(&memoryResource);
        }
        UnsynchronizedMemoryResource() : memoryResource(getPoolOptions()) {}
    private:
        static boost::container::pmr::pool_options getPoolOptions() noexcept {
            boost::container::pmr::pool_options po;
            return po;
        }
        boost::container::pmr::unsynchronized_pool_resource memoryResource;

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
