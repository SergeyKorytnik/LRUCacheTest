// LRUCacheV4.h : contains a definition of LRUCache class.
//   The LRUCache class implements a cache with LRU replacement policy.
//   The version of LRUCache uses boost::multi_index_container with sequenced index 
//   and with boost::multi_index::hashed_index if USE_UNORDERED_MAP is defined 
//   or with boost::multi_index::ordered_index otherwise.
//
// Written by Sergey Korytnik 

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#ifdef USE_UNORDERED_MAP
#include <boost/multi_index/hashed_index.hpp>
#else
#include <boost/multi_index/ordered_index.hpp>
#endif
#include <boost/multi_index/member.hpp>

template <typename KeyType, typename ValueType>
class LRUCache {
    struct Entry;
    using EntryMultiIndex =
        boost::multi_index_container <
            Entry,
            boost::multi_index::indexed_by<
                boost::multi_index::sequenced<>,
#ifdef USE_UNORDERED_MAP                
                boost::multi_index::hashed_unique<
#else
                boost::multi_index::ordered_unique<
#endif
                    boost::multi_index::member<Entry, KeyType, &Entry::key>
                >
            >
        >;
public:
    LRUCache(size_t cacheSize) : maxCacheSize(cacheSize){}

    std::pair<bool, ValueType> get(const KeyType& key) {
        auto& keyMap = entries.get<1>();
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            entries.relocate(entries.end(), entries.iterator_to(*l));
            return {true, l->value};
        }
        return {false, ValueType()};
    }

    void put(const KeyType& key, const ValueType& value) {
        put(std::move(KeyType(key)), std::move(ValueType(value)));
    }

    void put(KeyType&& key, ValueType&& value) {
        auto& keyMap = entries.get<1>();        
        auto l = keyMap.find(key);
        if (l != keyMap.end()) {
            l->value = std::move(value);
            entries.relocate(entries.end(), entries.iterator_to(*l));
            return;
        }

        if (entries.size() >= maxCacheSize) {
            entries.pop_front();
        }
        entries.emplace_back(std::move(key), std::move(value));
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
