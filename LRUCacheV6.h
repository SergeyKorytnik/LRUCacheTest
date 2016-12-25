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

template <typename KeyType, typename ValueType,
    typename Hasher = std::hash<KeyType>
>
class LRUCache {
public:
    using value_type = ValueType;
    using key_type = KeyType;
private:
    struct Entry;
    using QueueType = boost::intrusive::list<Entry>;
    using MapType = emilib::HashMap<KeyType, Entry,Hasher>;
public:
    LRUCache() = delete;
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;
    LRUCache(LRUCache&&) = default;
    LRUCache& operator=(LRUCache&&) = default;
    ~LRUCache() = default;

    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize)        
    {
        keyMap.reserve(cacheSize);
    }

    static const char* description() {
        if (std::is_same < Hasher, std::hash<KeyType>>::value) {
            return "LRUCache(emilib::HashMap + std::hash"
                " + boost::intrusive::list)";
        }
        else if (std::is_same < Hasher, boost::hash<KeyType>>::value) {
            return "LRUCache(emilib::HashMap + boost::hash"
                " + boost::intrusive::list)";
        }
        else {
            return "LRUCache(emilib::HashMap + unknown hash function"
                " + boost::intrusive::list)";
        }
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
        Entry* p = keyMap.try_get(key);
        if (p) { // the key already exist in the map
            p->value = std::move(value);
            pushToQueueEnd(*p);
            return false;
        }

        if (keyMap.size() >= maxCacheSize) {
            auto& e = lruQueue.front();
            lruQueue.pop_front();
            keyMap.erase(e.keyLocation);
        }

        auto l = keyMap.insert(key, std::move(value));
        assert(l.second == true);
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
