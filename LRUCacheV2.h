// LRUCacheV2.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses std::list together with std::unordered_map 
//   if USE_UNORDERED_MAP is defined or with std::map otherwise.
//
// Written by Sergey Korytnik 
//
// Note:
//    Special thanks to Fedor Chelnokov 
//    for suggestion to use only a map and a list. 
//    And also for reminding me about existence of list::splice operation.
//

//#define USE_UNORDERED_MAP
#include <list>
#ifdef USE_UNORDERED_MAP
#include <unordered_map>
#else
#include <map>
#endif
#include <cassert>

template <typename KeyType, typename ValueType>
class LRUCache {
    struct Entry;
#ifdef USE_UNORDERED_MAP 
    using MapType = std::unordered_map<KeyType, Entry>;
#else
    using MapType = std::map<KeyType, Entry>;
#endif
    using QueueType = std::list<typename MapType::iterator>;
public:
    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)
#ifdef USE_UNORDERED_MAP 
        , entries(2*cacheSize)
#endif
    {
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto l = entries.find(key);
        if (l != entries.end()) {
            pushToQueueEnd(l->second.queueLocation);
            return{ true, l->second.value };
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
        auto l = entries.try_emplace(std::move(key), std::move(value));
        if (l.second == false) { // the key already exist in the map
            l.first->second.value = std::move(value);
            pushToQueueEnd(l.first->second.queueLocation);
            return;
        }

        if (entries.size() > maxCacheSize) {
            auto eloc = lruQueue.front();
            lruQueue.pop_front();
            entries.erase(eloc);
        }

        lruQueue.push_back(l.first);
        l.first->second.queueLocation = --lruQueue.end();
    }
private:
    void pushToQueueEnd(typename QueueType::iterator it) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            it);
    }

    struct Entry {
        Entry(ValueType&& aValue) : value(std::move(aValue)) {}
        ValueType value;
        typename QueueType::iterator queueLocation;
    };
    MapType entries;
    QueueType lruQueue;
    const size_t maxCacheSize;
};
