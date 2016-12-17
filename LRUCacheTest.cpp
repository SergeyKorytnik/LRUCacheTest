// LRUCacheTest.cpp : Defines the entry point for the console application.
//

#ifdef TEST_LRU_CACHE_V1_MAP
#include "LRUCacheV1.h"
#elif defined(TEST_LRU_CACHE_V1_UNORDERED_MAP)
#define USE_UNORDERED_MAP
#include "LRUCacheV1.h"
#elif defined(TEST_LRU_CACHE_V2_MAP)
#include "LRUCacheV2.h"
#elif defined(TEST_LRU_CACHE_V2_UNORDERED_MAP)
#define USE_UNORDERED_MAP
#include "LRUCacheV2.h"
#elif defined(TEST_LRU_CACHE_V3_MAP)
#include "LRUCacheV3.h"
#elif defined(TEST_LRU_CACHE_V3_UNORDERED_MAP)
#define USE_UNORDERED_MAP
#include "LRUCacheV3.h"
#elif defined(TEST_LRU_CACHE_V4_MAP)
#include "LRUCacheV4.h"
#elif defined(TEST_LRU_CACHE_V4_UNORDERED_MAP)
#define USE_UNORDERED_MAP
#include "LRUCacheV4.h"
#elif defined(TEST_LRU_CACHE_V5_MAP)
#ifdef _MSC_VER 
#define WINDOWS 1
#define _CRT_NONSTDC_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif
#define LOGURU_IMPLEMENTATION 1
#include "LRUCacheV5.h"
#else
#error no version of LRUCache to be tested is specified
#endif

#include <cassert>
#include <random>
#include <iostream>
#include <chrono>
#include <algorithm>


namespace {
    void test1() {
        LRUCache<std::string, std::string> lruCache(4);
        std::vector<std::string> keys = { "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg" };
        std::vector<std::string> values = { "a1","b1", "c1", "d1", "e1", "f1","g1" };
        for (size_t i = 0; i < keys.size(); i++) {
            lruCache.put(keys[i], values[i]);
            auto res = lruCache.get(keys[i]);
            assert(res.first == true);
            assert(res.second == values[i]);
            assert(lruCache.get(keys[0]).first == true);
        }
        assert(lruCache.get(keys[0]).first == true);
        assert(lruCache.get(keys[0]).second == values[0]);
        assert(lruCache.get(keys[1]).first == false);
        assert(lruCache.get(keys[2]).first == false);
        assert(lruCache.get(keys[3]).first == false);
        assert(lruCache.get(keys[4]).first == true);
        assert(lruCache.get(keys[4]).second == values[4]);
        assert(lruCache.get(keys[5]).first == true);
        assert(lruCache.get(keys[5]).second == values[5]);
        assert(lruCache.get(keys[6]).first == true);
        assert(lruCache.get(keys[6]).second == values[6]);
        lruCache.put(keys[1], values[1]);
        assert(lruCache.get(keys[0]).first == false);
        lruCache.put(keys[1], values[0]);
        assert(lruCache.get(keys[1]).first == true);
        assert(lruCache.get(keys[1]).second == values[0]);
    }

    void test2() {
        LRUCache<std::string, size_t> lruCache(4);
        std::vector<std::string> keys = { "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg" };
        for (size_t i = 0; i < keys.size(); i++) {
            lruCache.put(keys[i], i);
            auto res = lruCache.get(keys[i]);
            assert(res.first == true);
            assert(res.second == i);
            assert(lruCache.get(keys[0]).first == true);
        }
        assert(lruCache.get(keys[0]).first == true);
        assert(lruCache.get(keys[0]).second == 0);
        assert(lruCache.get(keys[1]).first == false);
        assert(lruCache.get(keys[2]).first == false);
        assert(lruCache.get(keys[3]).first == false);
        assert(lruCache.get(keys[4]).first == true);
        assert(lruCache.get(keys[4]).second == 4);
        assert(lruCache.get(keys[5]).first == true);
        assert(lruCache.get(keys[5]).second == 5);
        assert(lruCache.get(keys[6]).first == true);
        assert(lruCache.get(keys[6]).second == 6);
        lruCache.put(keys[1], size_t(1));
        assert(lruCache.get(keys[0]).first == false);
        lruCache.put(keys[1], size_t(0));
        assert(lruCache.get(keys[1]).first == true);
        assert(lruCache.get(keys[1]).second == 0);
    }

    std::pair<size_t, size_t> performanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) {
        assert(samples.size() == sampleActions.size());

        LRUCache<size_t, size_t> lruCache(cacheSize);
        size_t numPutCalls = 0;
        size_t numGetCalls = 0;
        auto sampleIt = samples.begin();

        for (bool shouldAddNew : sampleActions) {
            size_t key = *sampleIt;
            ++sampleIt;
            size_t value = 2 * key;
            if (shouldAddNew) {
                numPutCalls += value % 1 + 1;
                lruCache.put(key, value);
            }
            else {
                auto res = lruCache.get(key);
                if (res.first == true && res.second != value)
                    throw std::runtime_error("invalid value in cache");
                numGetCalls += value % 1 + 1;
            }
        }
        return{ numPutCalls, numGetCalls };
    }
}


int main() {
    try {
        std::cout << "running basic sanity tests..";
        test1();
        std::cout << "..";
        test2();
        std::cout << "done\n" "generating random test sequence...";
        constexpr size_t cacheSize = 16 * 4096;
        constexpr size_t numTrials = 16 * 1000000;
        std::vector<size_t> samples(numTrials);
        std::vector<bool> sampleActions(numTrials);
        //std::random_device rd;
        std::mt19937 gen(0);
        std::binomial_distribution<size_t> binomial(4 * cacheSize, 0.89);
        std::bernoulli_distribution bernoulli(0.33);
        std::generate_n(samples.begin(), numTrials,
            [&]() { return binomial(gen); });
        std::generate_n(sampleActions.begin(), numTrials,
            [&]() { return bernoulli(gen); });

        std::cout << "done\n" "running the performance test...";
        auto startTime = std::chrono::high_resolution_clock::now();
        auto result = performanceTest(cacheSize, samples, sampleActions);
        auto finishTime = std::chrono::high_resolution_clock::now();
        std::cout << "done\n";
        using FpMilliseconds =
            std::chrono::duration<float, std::chrono::milliseconds::period>;
        std::cout << "\nThe performance test is complete in "
            << FpMilliseconds(finishTime - startTime).count() << " ms \n";
        std::cout << "The LRUCache.put method is called "
            << result.first << " times\n";
        std::cout << "The LRUCache.get method is called "
            << result.second << " times\n";
    }
    catch (std::exception& e) {
        std::cerr << "An exception has occurred: " << e.what() << '\n';
    }
    return 0;
}