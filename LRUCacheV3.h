// LRUCacheV3.h : contains a definition of LRUCacheV3 class.
//   The LRUCacheV3 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV3 uses boost::intrusive::list 
//   together with boost::intrusive::unordered_set or
//   boost::intrusive::set. The associative container for an efficient key 
//   lookup is selected depending on HashOption value.
//   If HashOption is Options::VoidHash then boost::intrusive::set is used.
//   Otherwise boost::intrusive::unordered_set is used together with the
//   provided hash function.
//
// Written by Sergey Korytnik 
#pragma once
#include <vector>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/set.hpp>
#include "LRUCacheOptions.h"

namespace LRUCache {

template <typename KeyType, typename ValueType,
    // boost::intrusive::map will be used if HashOption is void!
    typename HashOption = Options::StdHash
>
class LRUCacheV3 {
private:
    struct Entry;
    struct EntryKeyAccessor;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = boost::intrusive::unordered_set < Entry,
        // using std::hash rather than boost::hash for 
        // consistancy of comparison with other LRUCach implementations
        boost::intrusive::hash<typename HashOption::template type<KeyType> >,
        boost::intrusive::key_of_value<EntryKeyAccessor>,
        boost::intrusive::power_2_buckets<true>
    >;
    static size_t minPowerOfTwoLargerThan(size_t s) {
        size_t r = 4;
        while (r < s) {
            r *= 2;
        }
        return r;
    }
public:
    LRUCacheV3(size_t cacheSize)
        : maxCacheSize(cacheSize),
        buckets(minPowerOfTwoLargerThan(3 * cacheSize / 2 + 1)),
        keyMap(typename MapType::bucket_traits(buckets.data(), buckets.size()))
    {
        entries.reserve(cacheSize);
    }

    static std::string description() {
        return "LRUCacheV3(boost::intrusive::unordered_map(" 
            + HashOption::description()
            + "), boost::intrusive::list";
    }

    const ValueType* get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(*l);
            return &l->value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, const ValueType& value) {
        if (!put_CheckPhase(key, value))
            return false;
        put_CommitPhase(std::move(KeyType(key)), std::move(ValueType(value)));
        return true;
    }
    bool put(const KeyType& key, ValueType&& value) {
        if (!put_CheckPhase(key, std::move(value)))
            return false;
        put_CommitPhase(std::move(KeyType(key)), std::move(value));
        return true;
    }
    bool put(KeyType&& key, const ValueType& value) {
        if (!put_CheckPhase(key, value))
            return false;
        put_CommitPhase(std::move(key), std::move(ValueType(value)));
        return true;
    }

    bool put(KeyType&& key, ValueType&& value) {
        if (!put_CheckPhase(key, std::move(value)))
            return false;
        put_CommitPhase(std::move(key), std::move(value));
        return true;
    }
private:
    bool put_CheckPhase(const KeyType& key, const ValueType& value) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = value;
            pushToQueueEnd(*l);
            return false;
        }
        return true;
    }
    bool put_CheckPhase(const KeyType& key, ValueType&& value) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            pushToQueueEnd(*l);
            return false;
        }
        return true;
    }
    void put_CommitPhase(KeyType&& key, ValueType&& value) {
        Entry* e;
        if (entries.size() == maxCacheSize) {
            e = &lruQueue.front();
            e->key = std::move(key);
            e->value = std::move(value);
            pushToQueueEnd(*e);
            keyMap.erase(keyMap.iterator_to(*e));
        }
        else {
            entries.emplace_back(std::move(key), std::move(value));
            e = &entries.back();
            lruQueue.push_back(*e);
        }
        keyMap.insert(*e);
    }

    void pushToQueueEnd(Entry& e) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            QueueType::s_iterator_to(e));
    }

    struct Entry :
        public boost::intrusive::unordered_set_base_hook<>,
        public boost::intrusive::list_base_hook<> {

        Entry(KeyType&& aKey, ValueType&& aValue)
            : key(std::move(aKey)), value(std::move(aValue)) {}
        KeyType key;
        ValueType value;
    };

    struct EntryKeyAccessor {
        typedef KeyType type;

        const type & operator()(const Entry& v) const {
            return v.key;
        }
    };

    std::vector<Entry> entries;
    QueueType lruQueue;
    std::vector<typename MapType::bucket_type> buckets;
    MapType keyMap;
    const size_t maxCacheSize;
};

// the LRUCacheV3 class partial specialization uses 
// boost::intrusive::set instead of 
// boost::intrusive::unordered_set since void is passed instead of HashOption 
// template parameter (the last one).
template <typename KeyType, typename ValueType>
class LRUCacheV3<KeyType, ValueType, Options::VoidHash> 
{
    struct Entry;
    struct EntryKeyAccessor;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = boost::intrusive::set < Entry,
        boost::intrusive::key_of_value<EntryKeyAccessor>>;
public:
    LRUCacheV3() = delete;
    LRUCacheV3(const LRUCacheV3&) = delete;
    LRUCacheV3& operator=(const LRUCacheV3&) = delete;
    LRUCacheV3(LRUCacheV3&&) = default;
    LRUCacheV3& operator=(LRUCacheV3&&) = default;
    ~LRUCacheV3() = default;

    LRUCacheV3(size_t cacheSize)
        : maxCacheSize(cacheSize)
    {
        entries.reserve(cacheSize);
    }

    static constexpr const char* description() {
        return "LRUCacheV3(boost::intrusive::set, boost::intrusive::list)";
    }

    const ValueType* get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(*l);
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
        auto l = keyMap.lower_bound(key);
        if (l != keyMap.end() && l->key == key) {
            l->value = std::move(value);
            pushToQueueEnd(*l);
            return false;
        }
        Entry* e;
        if (entries.size() == maxCacheSize) {
            e = &lruQueue.front();
            e->key = std::move(key);
            e->value = std::move(value);
            pushToQueueEnd(*e);
            keyMap.erase(MapType::s_iterator_to(*e));
        }
        else {
            entries.emplace_back(std::move(key), std::move(value));
            e = &entries.back();
            lruQueue.push_back(*e);
        }
        keyMap.insert(l, *e);
        return true;
    }
private:
    void pushToQueueEnd(Entry& e) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            QueueType::s_iterator_to(e));
    }

    struct Entry :
        public boost::intrusive::set_base_hook<>,
        public boost::intrusive::list_base_hook<> {

        Entry(KeyType&& aKey, ValueType&& aValue)
            : key(std::move(aKey)), value(std::move(aValue)) {}
        KeyType key;
        ValueType value;
    };

    struct EntryKeyAccessor {
        typedef KeyType type;

        const type & operator()(const Entry& v) const {
            return v.key;
        }
    };

    std::vector<Entry> entries;
    QueueType lruQueue;
    MapType keyMap;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV3
