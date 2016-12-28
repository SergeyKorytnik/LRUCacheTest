// LRUCacheMapOptions.h : contains *Option classes configuring double linked list 
// containers that can be used as template parameters for LRUCacheV* 
// classes that are implementations of a cache class with LRU replacement 
// policy.  A list is used to maintain a LRU replacement queue.
// The following options are defined and implemented here:
//   StdList -- an option to use std::list with an allocator
//   BoostList -- an option to use boost::container::list with an allocator
//
// Written by Sergey Korytnik 

#pragma once

#include "LRUCacheOptions.h"
#include <boost/container/list.hpp>
#include <list>

namespace LRUCache {
namespace Options {

template <typename AllocatorOption = StdAllocator>
struct StdList {

    template <typename ListItem>
    using type = std::list<ListItem,
        typename AllocatorOption::template type<ListItem>
    >;

    static std::string description() {
        std::string a = AllocatorOption::description();
        if (a.empty()) {
            return "std::list";
        }
        else {
            return "std::list(" + a + ")";
        }
    }
};

template <typename AllocatorOption = StdAllocator>
struct BoostList {

    template <typename ListItem>
    using type = boost::container::list<ListItem,
        typename AllocatorOption::template type<ListItem>
    >;

    static std::string description() {
        std::string a = AllocatorOption::description();
        if (a.empty()) {
            return "boost::container::list";
        }
        else {
            return "boost::container::list(" + a + ")";
        }
    }
};

} // namespace Options {
} // namespace LRUCache {

