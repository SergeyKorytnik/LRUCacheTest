// LRUCacheV6.h : contains a definition of LRUCacheV6 class.
//   The LRUCacheV6 class implements a cache with LRU replacement policy.
//   The version of LRUCacheV6 uses boost::intrusive::list with
//   emilib::HashMap.
//
// Written by Sergey Korytnik 
#pragma once
#include "hash_map.hpp"
#include "LRUCacheOptions.h"
#include <vector>
#include <boost/intrusive/list.hpp>
#include <cassert>

namespace LRUCache {

template <typename KeyType, typename ValueType,
    typename HashOption = Options::StdHash
>
class LRUCacheV6 {
private:
    struct Entry;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = emilib::HashMap<KeyType, Entry,
        typename HashOption::template type<KeyType> >;

public:
    LRUCacheV6() = delete;
    LRUCacheV6(const LRUCacheV6&) = delete;
    LRUCacheV6& operator=(const LRUCacheV6&) = delete;
    LRUCacheV6(LRUCacheV6&&) = default;
    LRUCacheV6& operator=(LRUCacheV6&&) = default;
    ~LRUCacheV6() = default;

    LRUCacheV6(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        keyMap.reserve(2 * cacheSize);
    }

    static std::string description() {
        return std::string("LRUCacheV6(emilib::HashMap(")
            + HashOption::description()
            + "), boost::intrusive::list)";
    }

    const ValueType* get(const KeyType& key) {
        assert(keyMap.size() <= maxCacheSize);
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            pushToQueueEnd(l->second);
            return &l->second.value;
        }
        return nullptr;
    }

    bool put(const KeyType& key, ValueType value) {
        assert(keyMap.size() <= maxCacheSize);
        return finishPutOperation(
            keyMap.try_emplace(key, std::move(value)),
            std::move(value));
    }

    bool put(KeyType&& key, ValueType value) {
        assert(keyMap.size() <= maxCacheSize);
        return finishPutOperation(
            keyMap.try_emplace(key, std::move(value)),
            std::move(value));
    }

private:
    bool finishPutOperation(
        std::pair<typename MapType::iterator, bool> l, 
        ValueType&& value)
    {
        if (l.second == false) { // the key already exist in the map
            l.first->second.value = std::move(value);
            pushToQueueEnd(l.first->second);
            return false;
        }

        if (keyMap.size() > maxCacheSize) {
            auto& e = lruQueue.front();
            lruQueue.pop_front();
            keyMap.erase(e.keyLocation);
        }

        auto& e = l.first->second;
        e.keyLocation = l.first;
        lruQueue.push_back(e);
        return true;
    }
    void pushToQueueEnd(Entry& e) {
        lruQueue.splice(lruQueue.end(), lruQueue,
            QueueType::s_iterator_to(e));
    }

    struct Entry : public boost::intrusive::list_base_hook<> {
        //Entry(const Entry& rhs) 
        //    : boost::intrusive::list_base_hook<>(), 
        //      value(rhs.value) {}
        //Entry& operator =(Entry&& other) = delete;
        //Entry& operator =(const Entry& other) {
        //    if (&other == this)
        //        return *this;
        //    value = other.value;
        //    return *this;
        //}
    
        Entry(ValueType&& a_value)
          : value(std::move(a_value))
        {}
        ValueType value;
        typename MapType::iterator keyLocation;
    };
    MapType keyMap;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

} // namespace LRUCacheV5
