// LRUCacheV4.h : contains a definition of LRUCacheV4 class.
//   The LRUCacheV4 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV4 uses boost::multi_index_container with sequenced index 
//   and with boost::multi_index::ordered_index (if HashOption parameter is
//   Options::VoidHash) and boost::multi_index::hashed_index otherwise.
//
// Written by Sergey Korytnik 
#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include "LRUCacheOptions.h"


namespace LRUCache {

template <typename KeyType, typename ValueType, 
    // ordered_index will be used if HashOption is Options::VoidHash!
    typename HashOption = Options::StdHash,
    typename AllocatorOption = Options::StdAllocator
>
class LRUCacheV4 {
private:
    struct Entry;
    static constexpr bool use_unordered_map =
        !std::is_same<HashOption,Options::VoidHash>::value;

    using AllocatorType = typename AllocatorOption::template type<Entry>;
    using UnorderedIndexType = boost::multi_index::hashed_unique <
        boost::multi_index::member<Entry, KeyType, &Entry::key>,
        typename HashOption::template type<KeyType>
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
    LRUCacheV4(size_t cacheSize) : maxCacheSize(cacheSize) {}
    static std::string description() {
        std::string s = "LRUCacheV4(boost::multi_index_container(";
        if (std::is_same < HashOption, void >::value) {
            s += "ordered_unique";
        }
        else {
            s += "hashed_unique(";
            s += HashOption::description();
            s += ")";
        }
        std::string a = AllocatorOption::description();
        if (!a.empty()) {
            s += ", ";
            s += a;
        }
        s += ")";
        return s;
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
