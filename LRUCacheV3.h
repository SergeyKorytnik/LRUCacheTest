// LRUCacheV3.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses boost::intrusive::list together with boost::intrusive::unordered_set 
//   if USE_UNORDERED_MAP is defined or with boost::intrusive::set otherwise.
//
// Written by Sergey Korytnik 
#pragma once
#include <vector>
#include <boost/intrusive/list.hpp>
#include <boost/intrusive/unordered_set.hpp>
#include <boost/intrusive/set.hpp>

namespace LRUCacheV3 {
template <typename KeyType, typename ValueType, bool use_unordered_map>
class LRUCache;

template <typename KeyType, typename ValueType>
class LRUCache<KeyType, ValueType, false>
{
    struct Entry;
    struct EntryKeyAccessor;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = boost::intrusive::set < Entry,
        boost::intrusive::key_of_value<EntryKeyAccessor>>;
public:
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;

    LRUCache(size_t cacheSize) 
        : maxCacheSize(cacheSize) 
    {
        entries.reserve(cacheSize);
    }

    static constexpr const char* description() {
        return "LRUCache(boost::intrusive::set + boost::intrusive::list)";
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(*l);
            return{ true, l->value };
        }
        return{ false, ValueType() };
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
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            pushToQueueEnd(*l);
            return false;
        }
        if (entries.size() == maxCacheSize) {
            auto& e = lruQueue.front();
            e.key = std::move(key);
            e.value = std::move(value);
            pushToQueueEnd(e);
            keyMap.erase(MapType::s_iterator_to(e));
            keyMap.insert(l, e);
        }
        else {
            entries.emplace_back(std::move(key), std::move(value));
            auto& e = entries.back();
            lruQueue.push_back(e);
            keyMap.insert(l, e);
        }
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

template <typename KeyType, typename ValueType>
class LRUCache<KeyType, ValueType, true>
{
    struct Entry;
    struct EntryKeyAccessor;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = boost::intrusive::unordered_set < Entry,
        boost::intrusive::key_of_value<EntryKeyAccessor>,
        boost::intrusive::power_2_buckets<true>
    >;
public:
    LRUCache(size_t cacheSize)
        : maxCacheSize(cacheSize), buckets(2 * cacheSize),
        keyMap(MapType::bucket_traits(buckets.data(), buckets.size())) 
    {
        entries.reserve(cacheSize);
    }

    static constexpr const char* description() {        
        return "LRUCache(boost::intrusive::unordered_set + boost::intrusive::list)";
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(*l);
            return{ true, l->value };
        }
        return{ false, ValueType() };
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
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            pushToQueueEnd(*l);
            return false;
        }
        if (entries.size() == maxCacheSize) {
            auto& e = lruQueue.front();
            e.key = std::move(key);
            e.value = std::move(value);
            pushToQueueEnd(e);
            keyMap.erase(keyMap.iterator_to(e));
            keyMap.insert(e);
        }
        else {
            entries.emplace_back(std::move(key), std::move(value));
            auto& e = entries.back();
            lruQueue.push_back(e);
            keyMap.insert(e);
        }
        return true;
    }
private:
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

}// namespace LRUCacheV3
