// LRUCacheV3.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses boost::intrusive::list together with boost::intrusive::unordered_set 
//   if USE_UNORDERED_MAP is defined or with boost::intrusive::set otherwise.
//
// Written by Sergey Korytnik 

#include <vector>
#include <boost/intrusive/list.hpp>
#ifdef USE_UNORDERED_MAP
#include <boost/intrusive/unordered_set.hpp>
#else
#include <boost/intrusive/set.hpp>
#endif

template <typename KeyType, typename ValueType>
class LRUCache {
    struct Entry;
    struct EntryKeyAccessor;
    using QueueType = boost::intrusive::list<Entry>;
#ifndef USE_UNORDERED_MAP
    using MapType = boost::intrusive::set < Entry,
        boost::intrusive::key_of_value<EntryKeyAccessor>>;
#else
    using MapType = boost::intrusive::unordered_set < Entry,
        boost::intrusive::key_of_value<EntryKeyAccessor>,
        boost::intrusive::power_2_buckets<true>
    >;
#endif
public:
    LRUCache(size_t cacheSize) 
        : maxCacheSize(cacheSize) 
#ifdef USE_UNORDERED_MAP
        ,buckets(2 * cacheSize),
        keyMap(MapType::bucket_traits(buckets.data(), buckets.size()))
#endif
    {
        entries.reserve(cacheSize);
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(*l);
            return{ true, l->value };
        }
        return{ false, ValueType() };
    }

    void put(const KeyType& key, const ValueType& value) {
        put(std::move(KeyType(key)), std::move(ValueType(value)));
    }
    void put(const KeyType& key, ValueType&& value) {
        put(std::move(KeyType(key)), std::move(value));
    }
    void put(KeyType&& key, const ValueType& value) {
        put(std::move(key), std::move(ValueType(value)));
    }


    void put(KeyType&& key, ValueType&& value) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            pushToQueueEnd(*l);
            return;
        }
        if (entries.size() == maxCacheSize) {
            auto& e = lruQueue.front();
            e.key = std::move(key);
            e.value = std::move(value);
            pushToQueueEnd(e);
#ifdef USE_UNORDERED_MAP
            keyMap.erase(keyMap.iterator_to(e));
            keyMap.insert(e);
#else
            keyMap.erase(MapType::s_iterator_to(e));
            keyMap.insert(l, e);
#endif
        }
        else {
            entries.emplace_back(std::move(key), std::move(value));
            auto& e = entries.back();
            lruQueue.push_back(e);
#ifdef USE_UNORDERED_MAP
            keyMap.insert(e);
#else
            keyMap.insert(l, e);
#endif
        }
    }
private:
    void pushToQueueEnd(Entry& e) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            QueueType::s_iterator_to(e));
    }

    struct Entry : 
#ifdef USE_UNORDERED_MAP
        public boost::intrusive::unordered_set_base_hook<>,
#else
        public boost::intrusive::set_base_hook<>,
#endif
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
#ifdef USE_UNORDERED_MAP
    std::vector<typename MapType::bucket_type> buckets;
#endif
    MapType keyMap;
    const size_t maxCacheSize;
};

