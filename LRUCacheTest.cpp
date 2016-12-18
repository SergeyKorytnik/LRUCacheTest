// LRUCacheTest.cpp : Defines the entry point for the console application.
//

#ifdef _MSC_VER 
#define WINDOWS 1
#define _CRT_NONSTDC_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "LRUCacheV1.h"
#include "LRUCacheV2.h"
#include "LRUCacheV3.h"
#include "LRUCacheV4.h"
#define LOGURU_IMPLEMENTATION 1
#include "LRUCacheV5.h"
#include "LRUCacheV6.h"

#include <cassert>
#include <random>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <vector>
#include <algorithm>
#include <array>
#include <ctime>
#include <cstring>

class LRUCacheTest {
public:
    void check(bool value, const char* message)
    {
        if (!value) {
            throw std::runtime_error(message);
        }
    }
    virtual const char* getTestDescription() const = 0;
    virtual void runSanityTests() = 0;
    virtual void runPerformanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) = 0;
    void validateTestResults() {
        if (testResults.empty())
            return;
        const auto& r0 = testResults[0];
        for (const auto& r : testResults) {
            check(r0.cacheHitCount == r.cacheHitCount, 
                "cacheHitCount varies in different passes");
            check(r0.cacheMissCount == r.cacheMissCount,
                "cacheMissCount varies in different passes");
            check(r0.keyInsertionCount == r.keyInsertionCount,
                "keyInsertionCount varies in different passes");
            check(r0.totalEntryInsertionCount == r.totalEntryInsertionCount,
                "totalEntryInsertionCount varies in different passes");
        }
    }

    void printTestResults() {
        if (testResults.empty())
            return;
        float averageDuration = 0.0;
        for (auto& r : testResults) {
            averageDuration += r.testDuration;
        }
        averageDuration /= testResults.size();
        float avrAbsDeviation = 0.0;
        for (auto& r : testResults) {
            avrAbsDeviation += std::abs(r.testDuration - averageDuration);
        }
        avrAbsDeviation /= testResults.size();
        std::cout << getTestDescription();
        std::cout << ", "<< std::setprecision(6) << averageDuration;
        std::cout << ", " << std::setprecision(4) << avrAbsDeviation;
//        for (auto& r : testResults) {
//            std::cout << ", " << r.testDuration;
//        }
        std::cout << '\n';
    }

    struct PerformanceTestResults {
        PerformanceTestResults()
            : cacheHitCount(0), cacheMissCount(0),
            totalEntryInsertionCount(0), keyInsertionCount(0), testDuration(0) {}

        size_t cacheHitCount; // the number of calls of LRUCache.get that actually return a value
        size_t cacheMissCount; // the number of calls of LRUCache.get that returned {false,ValueType()}
        size_t totalEntryInsertionCount; //number of calls of LRUCache.put method        
        size_t keyInsertionCount; // number of calls of LRUCache.put method that returned true;
        float testDuration; // the test running time in miliseconds
    };
protected:
    std::vector<PerformanceTestResults> testResults;
};

namespace {
    static const std::array<std::string, 7> testKeys = { "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg" };
    static const std::array<std::string, 7> testValues = { "a1","b1", "c1", "d1", "e1", "f1","g1" };
}

template <template <typename,typename> typename LRUCache>
class LRUCacheTestImpl : public LRUCacheTest {
public:
    const char* getTestDescription() const override {
        return LRUCache<size_t, size_t>::description();
    }

    void runSanityTests() override
    {
        test1([&](size_t index) {return testValues[index]; });
        test1([&](size_t index) {return 2*index; });
    }

    void runPerformanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) override
    {
        auto startTime = std::chrono::high_resolution_clock::now();
        auto result = performanceTest(cacheSize, samples, sampleActions);
        auto finishTime = std::chrono::high_resolution_clock::now();
        using FpMilliseconds =
            std::chrono::duration<float, std::chrono::milliseconds::period>;
        result.testDuration = FpMilliseconds(finishTime - startTime).count();
        std::cout << result.testDuration << ", "
            << result.cacheHitCount << ", "
            << result.cacheMissCount << ", "
            << result.keyInsertionCount << ", "
            << result.totalEntryInsertionCount << '\n';
            testResults.emplace_back(result);
    }

private:
    template <typename IndexToValueMapping>
    void test1(IndexToValueMapping& indToValue) {
        LRUCache<std::string, decltype(indToValue(0)) > lruCache(4);
        for (size_t i = 0; i < testKeys.size(); i++) {
            lruCache.put(testKeys[i], indToValue(i));
            auto res = lruCache.get(testKeys[i]);
            check(res.first == true,
                "lruCache.get can't find the most recently added entry");
            check(res.second == indToValue(i),
                "lruCache.get returned incorrect value");
            check(lruCache.get(testKeys[0]).first == true,
                "lruCache.get can't find the second most recently used entry");
        }
        check(lruCache.get(testKeys[0]).second == indToValue(0),
            "lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[1]).first == false,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[2]).first == false,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[3]).first == false,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[4]).first == true,
            "lruCache.get can't find 4th MRU entry");
        check(lruCache.get(testKeys[4]).second == indToValue(4),
            "lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[5]).first == true,
            "lruCache.get can't find 3rd MRU entry");
        check(lruCache.get(testKeys[5]).second == indToValue(5),
            "lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[6]).first == true,
            "lruCache.get can't find 2nd MRU entry");
        check(lruCache.get(testKeys[6]).second == indToValue(6),
            "lruCache.get returned incorrect value");
        lruCache.put(testKeys[1], indToValue(1));
        check(lruCache.get(testKeys[0]).first == false,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        lruCache.put(testKeys[1], indToValue(0));
        check(lruCache.get(testKeys[1]).first == true,
            "lruCache.get can't find the most recently added entry");
        check(lruCache.get(testKeys[1]).second == indToValue(0),
            "lruCache.get returned incorrect value");
    }


    static PerformanceTestResults performanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) {
        assert(samples.size() == sampleActions.size());
        LRUCacheTest::PerformanceTestResults results;
        LRUCache<size_t, size_t> lruCache(cacheSize);
        auto sampleIt = samples.begin();

        for (bool shouldAddNew : sampleActions) {
            size_t key = *sampleIt;
            ++sampleIt;
            size_t value = 2 * key;
            if (shouldAddNew) {
                if (lruCache.put(key, value))
                    ++results.keyInsertionCount;
                ++results.totalEntryInsertionCount;
            }
            else {
                auto res = lruCache.get(key);
                if (res.first == true) {
                    if (res.second != value)
                        throw std::runtime_error("invalid value in cache");
                    ++results.cacheHitCount;
                }
                else
                    ++results.cacheMissCount;
            }
        }
        return results;
    }
};

template <typename KeyType, typename  ValueType>
using LRUCache_V1 = LRUCacheV1::LRUCache<KeyType, ValueType, false, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1u = LRUCacheV1::LRUCache<KeyType, ValueType, true, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1a = LRUCacheV1::LRUCache<KeyType, ValueType, false, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1ua = LRUCacheV1::LRUCache<KeyType, ValueType, true, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2 = LRUCacheV2::LRUCache<KeyType, ValueType, false, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2u = LRUCacheV2::LRUCache<KeyType, ValueType, true, false>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2a = LRUCacheV2::LRUCache<KeyType, ValueType, false, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2ua = LRUCacheV2::LRUCache<KeyType, ValueType, true, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V3 = LRUCacheV3::LRUCache<KeyType, ValueType, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V3u = LRUCacheV3::LRUCache<KeyType, ValueType, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4 = LRUCacheV4::LRUCache<KeyType, ValueType, false, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4u = LRUCacheV4::LRUCache<KeyType, ValueType, true, false>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4a = LRUCacheV4::LRUCache<KeyType, ValueType, false, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4ua = LRUCacheV4::LRUCache<KeyType, ValueType, true, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V5 = LRUCacheV5::LRUCache<KeyType, ValueType>;

template <typename KeyType, typename  ValueType>
using LRUCache_V6 = LRUCacheV6::LRUCache<KeyType, ValueType>;

int main() {
    try {
        std::array<size_t, 16> testPermutation;
        std::array<std::unique_ptr<LRUCacheTest>, testPermutation.size()> tests = {
            std::make_unique<LRUCacheTestImpl<LRUCache_V1> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V1u> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V1a> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V1ua> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V2> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V2u> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V2a> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V2ua> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V3> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V3u> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V4> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V4u> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V4a> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V4ua> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V5> >(),
            std::make_unique<LRUCacheTestImpl<LRUCache_V6> >(),
        };
        for (size_t i = 0; i < testPermutation.size(); i++)
            testPermutation[i] = i;

        std::time_t t = std::time(nullptr);
        std::cout << "local time: " << std::put_time(std::localtime(&t), "%F %T %z") << '\n'; 
#ifdef _MSC_VER
        std::cout << "Compiler: Microsoft Visual C++ (" << _MSC_FULL_VER << ")\n";
#ifdef _M_AMD64 
        std::cout << "Architecture: Windows x64" << '\n';
#endif
#ifdef _M_IX86  
        std::cout << "Architecture: Windows x86" << '\n';
#endif
#ifdef _M_ARM   
        std::cout << "Architecture: Windows ARM" << '\n';
#endif
#endif // _MSC_VER

        std::cout << "running basic sanity tests..\n";
        for (auto& t : tests) {
            std::cout << "testing " << t->getTestDescription() << '\n';
            t->runSanityTests();
        }
        std::cout << "done\n\n" "generating random test sequence...";
        constexpr size_t cacheSize = 2048;//16 * 4096;
        constexpr size_t numTrials = 16 * 1000000;
        std::vector<size_t> samples(numTrials);
        std::vector<bool> sampleActions(numTrials);
        //std::random_device rd;
        std::mt19937 gen(0);
        std::binomial_distribution<size_t> binomial(16*4096* cacheSize, 0.89);
        std::bernoulli_distribution bernoulli(0.33);///0.33);
        std::generate_n(samples.begin(), numTrials,
            [&]() { return binomial(gen); });
        std::generate_n(sampleActions.begin(), numTrials,
            [&]() { return bernoulli(gen); });
        std::cout << "done\nrunning performance tests..\n";
        for (int i = 0; i < 5; i++) {
            std::random_shuffle(testPermutation.begin(), testPermutation.end());
            std::cout << "Iteration #" << i << '\n';
            for (auto j: testPermutation) {
                auto& t = tests[j];
                std::cout << "testing " << t->getTestDescription() << '\n';
                t->runPerformanceTest(cacheSize, samples, sampleActions);
            }
        }
        std::cout << "done\nvalidating results consistency..\n";
        for (auto& t : tests) {
            t->validateTestResults();
        }

        std::cout << "done\nprinting final CSV table (all durations are in miliseconds) ..\n";
        for (auto& t : tests) {
            t->printTestResults();
        }
    }
    catch (std::exception& e) {
        std::cerr << "An exception has occurred: " << e.what() << '\n';
    }
    return 0;
}