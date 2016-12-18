// LRUCacheV6.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses boost::intrusive::list with
//   emilib::HashMap.
//
// Written by Sergey Korytnik 
#pragma once
#include "hash_map.hpp"
#include <vector>
#include <boost/intrusive/list.hpp>
#include <cassert>

namespace LRUCacheV6 {

template <typename KeyType, typename ValueType>
class LRUCache {
    struct Entry;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = emilib::HashMap<KeyType, Entry>;
public:
    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        keys.reserve(cacheSize);
    }

    static constexpr const char* description() {
        return "LRUCache(emilib::HashMap + boost::intrusive::list)";
    }

    std::pair<bool, ValueType> get(const KeyType& key) {
        assert(keys.size() <= maxCacheSize);
        auto l = keys.find(key);
        if (l != keys.end()) {
            pushToQueueEnd(l->second);
            return{ true, l->second.value };
        }
        return{ false, ValueType() };
    }

    bool put(const KeyType& key, const ValueType& value) {
        assert(keys.size() <= maxCacheSize);
        auto l = keys.insert(key, Entry(value));
        if (l.second == false) { // the key already exist in the map
            auto& e = l.first->second;
            e.value = value;
            pushToQueueEnd(e);
            return false;
        }

        if (keys.size() > maxCacheSize) {
            auto& e = lruQueue.front();
            lruQueue.pop_front();
            keys.erase(e.keyLocation);
        }

        auto& e = l.first->second;
        e.keyLocation = l.first;
        lruQueue.push_back(e);
        return true;
    }

private:
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
    
        Entry(const ValueType& a_value)
          : value(a_value)
        {}
        ValueType value;
        typename MapType::iterator keyLocation;
    };
    MapType keys;
    QueueType lruQueue;
    const size_t maxCacheSize;
};

} // namespace LRUCacheV5
