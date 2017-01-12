// LRUCacheV2.h : contains a definition of LRUCacheV2 class.
//   The LRUCacheV2 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV2 uses stl compatible double connected list 
//   together with an associative container for efficient key lookup. 
//
// Written by Sergey Korytnik 
//
// Note:
//    Special thanks to Fedor Chelnokov 
//    for suggestion to use only a map and a list; 
//    And also for reminding me about existence of list::splice operation. :-)
//
#pragma once
#include "LRUCacheMapOptions.h"
#include "LRUCacheListOptions.h"
#include <cassert>


namespace LRUCache {

template <typename KeyType, typename ValueType, 
    typename MapOption = Options::StdMap<>,  // see LRUCacheMapOptions.h for other options
    typename ListOption = Options::StdList<> // see LRUCacheListOptions.h for other options
>
class LRUCacheV2 {
private:
    struct Entry;
    using MapType = typename MapOption::template type<KeyType, Entry>;

    struct QueueItem; 
    using QueueType = typename ListOption::template type<QueueItem>;
public:
    LRUCacheV2() = delete;
    LRUCacheV2(const LRUCacheV2&) = delete;
    LRUCacheV2& operator=(const LRUCacheV2&) = delete;
    LRUCacheV2(LRUCacheV2&&) = default;
    LRUCacheV2& operator=(LRUCacheV2&&) = default;
    ~LRUCacheV2() = default;
    LRUCacheV2(size_t cacheSize)
        : maxCacheSize(cacheSize), keyMap(2*cacheSize,ao), 
        lruQueue(ao.template getAllocator<typename QueueType::value_type>())
    {}

    const ValueType* get(const KeyType& key) {
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(l->second.queueLocation);
            return &l->second.queueLocation->value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, const ValueType& value) {
        return finishPutOperation(
            keyMap.try_emplace(key, Entry()),
            std::move(ValueType(value)));
    }
    bool put(const KeyType& key, ValueType&& value) {
        return finishPutOperation(
            keyMap.try_emplace(key, Entry()),
            std::move(value));
    }

    bool put(KeyType&& key, const ValueType& value) {
        return finishPutOperation(
            keyMap.try_emplace(std::move(key), Entry()),
            std::move(ValueType(value)));
    }

    bool put(KeyType&& key, ValueType&& value) {
        return finishPutOperation(
            keyMap.try_emplace(std::move(key), Entry()),
            std::move(value));
    }

    static std::string description() {
        std::string s = "LRUCacheV2(";
        s += MapOption::description();
        s += ", ";
        s += ListOption::description();
        s += ")";
        return s;
    }

private:
    bool finishPutOperation(
        std::pair<typename MapType::iterator, bool> l, ValueType&& value) {
        if (l.second == false) { // the key already exist in the map
            l.first->second.queueLocation->value = std::move(value);
            pushToQueueEnd(l.first->second.queueLocation);
            return false;
        }
        if (keyMap.size() > maxCacheSize) {
            auto eloc = lruQueue.front().mapLocation;
            lruQueue.pop_front();
            keyMap.erase(eloc);
        }
        lruQueue.emplace_back(std::move(value), l.first);
        l.first->second.queueLocation = --lruQueue.end();
        return true;
    }

    void pushToQueueEnd(typename QueueType::iterator it) {
        lruQueue.splice(lruQueue.end(), lruQueue, it);
    }

    struct Entry {
        typename QueueType::iterator queueLocation;
    };

    struct QueueItem {
        QueueItem(ValueType&& aValue,
            const typename MapType::iterator& it) 
        : value(std::move(aValue)), mapLocation(it) {}
        ValueType value;
        typename MapType::iterator mapLocation;
    };
    typename MapOption::MyAllocatorOption ao;
    MapType keyMap;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

}// namespace LRUCacheV2
