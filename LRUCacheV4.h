// LRUCacheV4.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses boost::multi_index_container with sequenced index 
//   and with boost::multi_index::hashed_index if USE_UNORDERED_MAP is defined 
//   or with boost::multi_index::ordered_index otherwise.
//
// Written by Sergey Korytnik 
#pragma once

#include <functional>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/pool/pool_alloc.hpp>


namespace LRUCacheV4 {

template <typename KeyType, typename ValueType, 
    // ordered_index will be used if Hasher is void!
    class Hasher = std::hash<KeyType>,
    bool use_fast_allocator = false
>
class LRUCache {
public:
    using value_type = ValueType;
    using key_type = KeyType;
private:
    struct Entry;
    static constexpr bool use_unordered_map =
        !std::is_void<Hasher>::value;

    using AllocatorType = typename std::conditional<use_fast_allocator,
        boost::fast_pool_allocator<Entry>,
        std::allocator<Entry>
    >::type;

    using UnorderedIndexType = boost::multi_index::hashed_unique<
        boost::multi_index::member<Entry, KeyType, &Entry::key>,
        // using std::hash rather than boost::hash for 
        // consistancy of comparison with other LRUCach implementations
        Hasher
    >;
    using OrderedIndexType = boost::multi_index::ordered_unique<
        boost::multi_index::member<Entry, KeyType, &Entry::key>
    >;
    using IndexType = typename std::conditional<use_unordered_map,
            UnorderedIndexType, OrderedIndexType>::type;

    using EntryMultiIndex =
        boost::multi_index_container <
            Entry,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
                IndexType
            >,
            AllocatorType
        >;
public:
    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize) {}

    static constexpr const char* description() {
        return use_unordered_map ?
            (use_fast_allocator ?
                "LRUCache(boost::multi_index_container"
                " + hashed_unique + boost::fast_pool_allocator)"
                :
                "LRUCache(boost::multi_index_container"
                " + hashed_unique + std::allocator)"
                )
            :
            (use_fast_allocator ?
                "LRUCache(boost::multi_index_container"
                " + ordered_unique + boost::fast_pool_allocator)"
                :
                "LRUCache(boost::multi_index_container"
                " + ordered_unique + std::allocator)"
                )
            ;
    }

    const ValueType* get(const KeyType& key) {
        auto& keyMap = entries.template get<1>();
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            entries.relocate(entries.end(), entries.iterator_to(*l));
            return &l->value;
        }
        return nullptr;
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
        auto& keyMap = entries.template get<1>();
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            entries.relocate(entries.end(), entries.iterator_to(*l));
            return false;
        }

        if (entries.size() >= maxCacheSize) {
            entries.pop_front();
        }
        entries.emplace_back(std::move(key), std::move(value));
        return true;
    }
private:
    struct Entry {
        Entry(KeyType&& aKey, ValueType&& aValue)
            : key(std::move(aKey)), value(std::move(aValue)) {}
        KeyType key;
        // to be able to modify a field in multi_index_container it must be
        // marked as mutable!
        // It is a trick from 
        // http://www.boost.org/doc/libs/1_62_0/libs/multi_index/doc/tutorial/techniques.html#emulate_assoc_containers
        mutable ValueType value;
    };

    EntryMultiIndex entries;
    const size_t maxCacheSize;
};

} // namespace LRUCacheV4
