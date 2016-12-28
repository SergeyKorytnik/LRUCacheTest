// LRUCacheTest.cpp : Defines the entry point for the console application.
// The application tests both correctness and performance of 
// various implementations of a cache class with LRU replacement policy.
//
// Written by Sergey Korytnik 
#ifdef _MSC_VER 
#define WINDOWS 1
#define _CRT_NONSTDC_NO_WARNINGS 
#define _CRT_SECURE_NO_WARNINGS
#endif
#define LOGURU_IMPLEMENTATION 1

#include "LRUCacheV1.h"
#include "LRUCacheV2.h"
#include "LRUCacheV3.h"
#include "LRUCacheV4.h"
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
    struct PerformanceTestResults {
        PerformanceTestResults()
            : cacheHitCount(0), cacheMissCount(0),
            totalEntryInsertionCount(0), keyInsertionCount(0), testDuration(0) {}

        size_t cacheHitCount; // the number of calls of LRUCache.get that actually return a value
        size_t cacheMissCount; // the number of calls of LRUCache.get that returned {false,ValueType()}
        size_t totalEntryInsertionCount; //number of calls of LRUCache.put method        
        size_t keyInsertionCount; // number of calls of LRUCache.put method that returned true;
        float testDuration; // the test running time in miliseconds
        void printStatistics()const {
            std::cout 
                << "\tCacheHitCount:          " << cacheHitCount << '\n'
                << "\tCacheMissCount:         " << cacheMissCount << '\n'
                << "\tKeyInsertionCount:      " << keyInsertionCount << '\n'
                << "\tTotalPutOperationCount: " << totalEntryInsertionCount << '\n';
        }
    };

    static void check(bool value, const char* message)
    {
        if (!value) {
            throw std::runtime_error(message);
        }
    }
    virtual std::string getTestDescription() const = 0;
    virtual void runSanityTests() = 0;
    virtual void runPerformanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) = 0;

    void validateTestResults(
        const std::vector<PerformanceTestResults>& testResults,
        const PerformanceTestResults& r0
    )const {
        if (testResults.empty())
            return;
        for (const auto& r : testResults) {
            if(r0.cacheHitCount != r.cacheHitCount) {
                std::cout << getTestDescription() << '\n'
                     << r.cacheHitCount << '\n'
                    << r0.cacheHitCount << '\n';
            }
            check(r0.cacheHitCount == r.cacheHitCount, 
                "cacheHitCount varies in different tests");
            check(r0.cacheMissCount == r.cacheMissCount,
                "cacheMissCount varies in different tests");
            check(r0.keyInsertionCount == r.keyInsertionCount,
                "keyInsertionCount varies in different tests");
            check(r0.totalEntryInsertionCount == r.totalEntryInsertionCount,
                "totalEntryInsertionCount varies in different tests");
        }
    }

    static void printTestResults(
        const std::vector<PerformanceTestResults>& testResults
    ) {
        float averageDuration = 0.0;
        for (auto& r : testResults) {
            averageDuration += r.testDuration;
        }
        size_t numSamples = testResults.size();
        float stdDeviation = 0.0;
        if (numSamples > 1) {
            averageDuration /= numSamples;
            for (auto& r : testResults) {
                auto d = r.testDuration - averageDuration;
                stdDeviation += d * d;
            }
            stdDeviation = std::sqrt(stdDeviation / (numSamples - 1));
        }
        std::cout << std::setprecision(6) << averageDuration;
        std::cout << '\t' << std::setprecision(4) << stdDeviation;
//        for (auto& r : testResults) {
//            std::cout << ", " << r.testDuration;
//        }
    }

    const std::vector<PerformanceTestResults>& getTestResults1() const {
        return testResults[0];
    }
    const std::vector<PerformanceTestResults>& getTestResults2() const {
        return testResults[1];
    }

protected:
    // 0th for LRUCache<size_t,size_t> 1st for LRUCache<std::string,std::string>
    std::vector<PerformanceTestResults> testResults[2]; 
};

namespace {
    static const std::array<std::string, 7> testKeys = { "aaa", "bbb", "ccc", "ddd", "eee", "fff", "ggg" };
    static const std::array<std::string, 7> testValues = { "a1","b1", "c1", "d1", "e1", "f1","g1" };
}

template <template <class,class> class LRUCache>
class LRUCacheTestImpl : public LRUCacheTest {
public:
    std::string getTestDescription() const override {
        return LRUCache<size_t, size_t>::description();
    }

    void runSanityTests() override
    {
        test1([&](size_t index)->const std::string& {
                return testValues[index]; 
            },
            std::equal_to<std::string>());
        test1([&](size_t index) {return 2*index; }, std::equal_to<size_t>());
        test1([&](size_t index)->std::unique_ptr<double> {
                return std::make_unique<double>(
                    2.0 * static_cast<double>(index) );
            },
            [](const auto& rhs, const auto& lhs)->bool {
                return *rhs == *lhs;
            }
        );
    }

    void runPerformanceTest(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) override
    {
        auto tests = { performanceTest1 , performanceTest2 };
        size_t i = 0;
        for (auto perfTest : tests) {
            auto startTime = std::chrono::high_resolution_clock::now();
            auto result = perfTest(cacheSize, samples, sampleActions);
            auto finishTime = std::chrono::high_resolution_clock::now();
            using FpMilliseconds =
                std::chrono::duration<float, std::chrono::milliseconds::period>;
            result.testDuration = FpMilliseconds(finishTime - startTime).count();
            testResults[i++].emplace_back(result);
        }
    }

private:
    template <typename IndexToValueMapping, typename EqualFunc>
    void test1(IndexToValueMapping indToValue, EqualFunc equalFunc) {
        LRUCache<std::string, 
            typename std::remove_cv<
                typename std::remove_reference<
                    decltype(indToValue(0))>::type>::type > lruCache(4);
        for (size_t i = 0; i < testKeys.size(); i++) {
            lruCache.put(testKeys[i], indToValue(i));
            auto res = lruCache.get(testKeys[i]);
            check(res != nullptr,
                "lruCache.get can't find the most recently added entry");
            check(equalFunc(*res, indToValue(i)),
                "1: lruCache.get returned incorrect value");
            check(lruCache.get(testKeys[0]) != nullptr,
                "lruCache.get can't find the second most recently used entry");
        }
        check(equalFunc(*lruCache.get(testKeys[0]), indToValue(0)),
            "2: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[1]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[2]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[3]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[4]) != nullptr,
            "lruCache.get can't find 4th MRU entry");
        check(equalFunc(*lruCache.get(testKeys[4]), indToValue(4)),
            "3: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[5]) != nullptr,
            "lruCache.get can't find 3rd MRU entry");
        check(equalFunc(*lruCache.get(testKeys[5]), indToValue(5)),
            "4: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[6]) != nullptr,
            "lruCache.get can't find 2nd MRU entry");
        check(equalFunc(*lruCache.get(testKeys[6]), indToValue(6)),
            "5: lruCache.get returned incorrect value");
        lruCache.put(testKeys[1], indToValue(1));
        check(lruCache.get(testKeys[0]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        lruCache.put(testKeys[1], indToValue(0));
        check(lruCache.get(testKeys[1]) != nullptr,
            "lruCache.get can't find the most recently added entry");
        check(equalFunc(*lruCache.get(testKeys[1]), indToValue(0)),
            "6: lruCache.get returned incorrect value");
    }


    static PerformanceTestResults performanceTest1(
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
                if (res) {
                    if (*res != value)
                        throw std::runtime_error("invalid value in cache");
                    ++results.cacheHitCount;
                }
                else
                    ++results.cacheMissCount;
            }
        }
        return results;
    }
    // The differences from performanceTest1:
    // 1) LRUCache<std::string,std::string> instead of LRUCache<size_t,size_t>
    // 2) Just the first 10 percent of the sequence are used.
    static PerformanceTestResults performanceTest2(
        size_t cacheSize,
        const std::vector<size_t>& samples,
        const std::vector<bool>& sampleActions
    ) {
        assert(samples.size() == sampleActions.size());
        LRUCacheTest::PerformanceTestResults results;
        LRUCache<std::string, std::string> lruCache(cacheSize);
        auto sampleIt = samples.begin();
        auto sampleActionIt = sampleActions.begin();        
        size_t numTrials = sampleActions.size() / 10;

        for (size_t i = 0; i < numTrials; ++i) {
            bool shouldAddNew = *sampleActionIt;
            ++sampleActionIt;
            size_t key = *sampleIt;
            ++sampleIt;
            size_t value = 2 * key;
            if (shouldAddNew) {
                if (lruCache.put(std::to_string(key), std::to_string(value)))
                    ++results.keyInsertionCount;
                ++results.totalEntryInsertionCount;
            }
            else {
                auto res = lruCache.get(std::to_string(key));
                if (res) {
                    if (std::stoul(*res) != value)
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

// generates a sequences of keys of type size_t using std::binomial_distribution
// and a sequence of boolean values using a std::bernoulli_distribution
// Returns generated keys and bool values as a pair of std::vector objects

std::pair<std::vector<size_t>, std::vector<bool>> generateTestSequence(
    size_t numTrials, // the length of sequences to be generated
    unsigned int seed, // the initialization parameter for random generator
    size_t maxKeyValueToGenerate, // =number of trials for binomial_distribution
    double binomialTrialSuccessProbability,
    double bernoulliTrialSuccessProbability
)
{
    std::vector<size_t> samples(numTrials);
    std::vector<bool> sampleActions(numTrials);
    //std::random_device rd;
    std::mt19937 gen(seed);
    size_t meanValue = static_cast<size_t>(
          maxKeyValueToGenerate * binomialTrialSuccessProbability);
    size_t dispersion = static_cast<size_t>(std::sqrt(
          maxKeyValueToGenerate * binomialTrialSuccessProbability
          * (1.0 - binomialTrialSuccessProbability)));
    size_t minValue = meanValue - 5 * dispersion;
    size_t maxValue = meanValue + 5 * dispersion;
    std::cout << "generating " << numTrials
        //<< " keys using binomial_distribution("        
        //<< maxKeyValueToGenerate << ", "
        //<< binomialTrialSuccessProbability << ")\n";
        //<< " keys using normal_distribution( "
        //<< meanValue << ", "
        //<< dispersion << ")\n";
        << " keys using uniform_int_distribution( "
        << minValue << ", "
        << maxValue << ")\n";
    //std::binomial_distribution<size_t> binomial(
    //    maxKeyValueToGenerate, binomialTrialSuccessProbability);
    //std::normal_distribution<size_t> normal(
    //      meanValue, dispersion);
    std::uniform_int_distribution<size_t> uniform(
        minValue, maxValue);
    std::generate_n(samples.begin(), numTrials,
        [&]() { return uniform(gen); });
    std::cout << "generating put/get flag sequence of " << numTrials
        << " booleans using bernoulli_distribution("
        << bernoulliTrialSuccessProbability << ")\n";
    std::bernoulli_distribution bernoulli(bernoulliTrialSuccessProbability);
    std::generate_n(sampleActions.begin(), numTrials,
        [&]() { return bernoulli(gen); });
    return{ samples,sampleActions };
}

// The legend for types below 
//    LRUCache_VXabcd
//       X - 1..6 (version)
//       a - o/u (ordered map or unordered map)
//       b - v/s/b (no hash, std::hash, boost::hash)
//       c - s/f (std::allocator, boost::fast_pool_allocator)
//       d - s/b (std::map + std::unordered_map + std::list vs 
//                boost::container::map + boost::unordered + boost::container::list)
//  

template <typename KeyType, typename  ValueType>
using LRUCache_V1ovss = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdMap<>>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1usss = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdUnorderedMap<>>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1ubss = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdUnorderedMap<
    LRUCache::Options::BoostHash> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ovfs = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdMap<
        LRUCache::Options::FastPoolAllocator> >;
template <typename KeyType, typename  ValueType>
using LRUCache_V1usfs = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdUnorderedMap<
        LRUCache::Options::StdHash,
        LRUCache::Options::FastPoolAllocator> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ubfs = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::StdUnorderedMap<
    LRUCache::Options::BoostHash,
    LRUCache::Options::FastPoolAllocator> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ovsb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostMap<>>;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ussb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostUnorderedMap<>>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1ubsb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostUnorderedMap<
    LRUCache::Options::BoostHash> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ovfb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostMap<
        LRUCache::Options::FastPoolAllocator> >;
template <typename KeyType, typename  ValueType>
using LRUCache_V1usfb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostUnorderedMap<
    LRUCache::Options::StdHash,
    LRUCache::Options::FastPoolAllocator> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1ubfb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::BoostUnorderedMap<
    LRUCache::Options::BoostHash,
    LRUCache::Options::FastPoolAllocator> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1es = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::EmilibHashMap<
    LRUCache::Options::StdHash> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V1eb = LRUCache::LRUCacheV1<
    KeyType, ValueType, LRUCache::Options::EmilibHashMap<
    LRUCache::Options::BoostHash> >;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ovss = LRUCache::LRUCacheV2<
    KeyType, ValueType, 
    LRUCache::Options::StdMap<>, 
    LRUCache::Options::StdList<>
>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2usss = LRUCache::LRUCacheV2<
    KeyType, ValueType, 
    LRUCache::Options::StdUnorderedMap<>, 
    LRUCache::Options::StdList<>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ubss = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::StdUnorderedMap<LRUCache::Options::BoostHash>,
    LRUCache::Options::StdList<>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ovfs = LRUCache::LRUCacheV2<
    KeyType, ValueType, 
    LRUCache::Options::StdMap<LRUCache::Options::FastPoolAllocator>,
    LRUCache::Options::StdList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2usfs = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::StdUnorderedMap<
        LRUCache::Options::StdHash,
        LRUCache::Options::FastPoolAllocator
    >,
    LRUCache::Options::StdList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ubfs = LRUCache::LRUCacheV2<
    KeyType, ValueType, 
    LRUCache::Options::StdUnorderedMap<
        LRUCache::Options::BoostHash,
        LRUCache::Options::FastPoolAllocator
    >,
    LRUCache::Options::StdList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ovsb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::BoostMap<LRUCache::Options::StdAllocator>,
    LRUCache::Options::BoostList<LRUCache::Options::StdAllocator>
>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2ussb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::BoostUnorderedMap<
        LRUCache::Options::StdHash,
        LRUCache::Options::StdAllocator 
    >,
    LRUCache::Options::BoostList<LRUCache::Options::StdAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ubsb = LRUCache::LRUCacheV2<
    KeyType, ValueType, 
    LRUCache::Options::BoostUnorderedMap<
        LRUCache::Options::BoostHash,
        LRUCache::Options::StdAllocator
    >,
    LRUCache::Options::BoostList<LRUCache::Options::StdAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ovfb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::BoostMap<LRUCache::Options::FastPoolAllocator>,
    LRUCache::Options::BoostList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2usfb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::BoostUnorderedMap<
        LRUCache::Options::StdHash,
        LRUCache::Options::FastPoolAllocator
    >,
    LRUCache::Options::BoostList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2ubfb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::BoostUnorderedMap<
        LRUCache::Options::BoostHash,
        LRUCache::Options::FastPoolAllocator
    >,
    LRUCache::Options::BoostList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2es = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::EmilibHashMap<LRUCache::Options::StdHash>,
    LRUCache::Options::BoostList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2eb = LRUCache::LRUCacheV2<
    KeyType, ValueType,
    LRUCache::Options::EmilibHashMap<LRUCache::Options::BoostHash>,
    LRUCache::Options::BoostList<LRUCache::Options::FastPoolAllocator>
>;

template <typename KeyType, typename  ValueType>
using LRUCache_V3ov = LRUCache::LRUCacheV3<KeyType, ValueType, 
    LRUCache::Options::VoidHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V3us = LRUCache::LRUCacheV3<
    KeyType, ValueType, LRUCache::Options::StdHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V3ub = LRUCache::LRUCacheV3<
    KeyType, ValueType, LRUCache::Options::BoostHash>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4ovs = LRUCache::LRUCacheV4<KeyType, ValueType,
    LRUCache::Options::VoidHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4uss = LRUCache::LRUCacheV4<
    KeyType, ValueType, LRUCache::Options::StdHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4ubs = LRUCache::LRUCacheV4<
    KeyType, ValueType, LRUCache::Options::BoostHash>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4ovf = LRUCache::LRUCacheV4<KeyType, ValueType,
    LRUCache::Options::VoidHash,
    LRUCache::Options::FastPoolAllocator>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4usf = LRUCache::LRUCacheV4<
    KeyType, ValueType, LRUCache::Options::StdHash,
    LRUCache::Options::FastPoolAllocator
>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4ubf = LRUCache::LRUCacheV4<
    KeyType, ValueType, LRUCache::Options::BoostHash,
    LRUCache::Options::FastPoolAllocator
>;


template <typename KeyType, typename  ValueType>
using LRUCache_V5s = LRUCache::LRUCacheV5<
    KeyType, ValueType, LRUCache::Options::StdHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V5b = LRUCache::LRUCacheV5<
    KeyType, ValueType, LRUCache::Options::BoostHash>;

template <typename KeyType, typename  ValueType>
using LRUCache_V6s = LRUCache::LRUCacheV6<
    KeyType, ValueType, LRUCache::Options::StdHash>;
template <typename KeyType, typename  ValueType>
using LRUCache_V6b = LRUCache::LRUCacheV6<
    KeyType, ValueType, LRUCache::Options::BoostHash>;

std::vector<std::unique_ptr<LRUCacheTest>> constructTestVector() {
    std::unique_ptr<LRUCacheTest> init_list[] = {
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ovsb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ovfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ovss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ovfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ovsb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ovfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ovss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ovfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V3ov> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4ovs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4ovf> >(),
        // hash based implementations of LRUCache are below
        std::make_unique<LRUCacheTestImpl<LRUCache_V1usss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ubss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1usfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ubfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ussb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ubsb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1usfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1ubfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2usss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ubss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2usfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ubfs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ussb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ubsb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2usfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2ubfb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V3us> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V3ub> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4uss> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4ubs> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4usf> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V4ubf> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V5s> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V5b> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V6s> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V6b> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1es> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V1eb> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2es> >(),
        std::make_unique<LRUCacheTestImpl<LRUCache_V2eb> >(),
    };
    return std::vector<std::unique_ptr<LRUCacheTest>>(
        std::make_move_iterator(std::begin(init_list)),
        std::make_move_iterator(std::end(init_list)));
}

void runPerformanceTests(
    std::vector<std::unique_ptr<LRUCacheTest>>& tests,
    size_t cacheSize,
    size_t numTrials, // the length of sequences to be generated
    size_t maxKeyValueToGenerate, // =number of trials for binomial_distribution
    double binomialTrialSuccessProbability,
    double bernoulliTrialSuccessProbability
) {

    std::cout << "generating random test sequence...\n";
    auto testSeq = generateTestSequence(
        numTrials, 0, maxKeyValueToGenerate, 
        binomialTrialSuccessProbability, bernoulliTrialSuccessProbability);

    std::cout << "done\nrunning performance tests for cacheSize = "
        << cacheSize;
    std::vector<size_t> testPermutation(tests.size());
    for (size_t i = 0; i < testPermutation.size(); i++)
        testPermutation[i] = i;
    for (int i = 0; i < 5; i++) {
        std::shuffle(testPermutation.begin(), testPermutation.end(), 
            std::mt19937(std::random_device()()));
        std::cout << "\nIteration #" << i;
        for (auto j : testPermutation) {
            auto& t = tests[j];
            std::cout << '.';
            //std::cout << "testing " << t->getTestDescription() << '\n';
            t->runPerformanceTest(cacheSize, testSeq.first, testSeq.second);
        }
    }
    
    if (!tests.front()->getTestResults1().empty()) {
        std::cout << "\ndone\nvalidating LRUCache<size_t,size_t> test results consistency..\n";
        auto& theResultToCompareWith1 = tests.front()->getTestResults1()[0];
        for (auto& t : tests) {
            t->validateTestResults(t->getTestResults1(), theResultToCompareWith1);
        }
        std::cout << "\ndone\n";
        std::cout << "all LRUCache<size_t,size_t> tests reported the following statistic:\n";
        theResultToCompareWith1.printStatistics();
    }

    if (!tests.front()->getTestResults2().empty()) {
        std::cout << "validating LRUCache<std::string,std::string> test results consistency..\n";
        auto& theResultToCompareWith2 = tests.front()->getTestResults2()[0];
        for (auto& t : tests) {
            t->validateTestResults(t->getTestResults2(), theResultToCompareWith2);
        }
        std::cout << "done\n";
        std::cout << "all LRUCache<string,std::string> tests reported the following statistic:\n";
        theResultToCompareWith2.printStatistics();
    }
}

int main() {
    try {

        std::time_t t = std::time(nullptr);
        std::cout << "local time: " << std::put_time(std::localtime(&t), "%F %T %z") << '\n'; 
#ifdef __clang__
        std::cout << "Compiler: Clang(" << __clang__  << ' ' 
            << __clang_version__ << ")\n";
#elif defined(__GNUC__)
        std::cout << "Compiler: GNU C++ (" << __GNUC__ << '.' << __GNUC_MINOR__ << ")\n";
#endif
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

        auto tests = constructTestVector();
        std::cout << "running basic sanity tests..\n";
        for (auto& t : tests) {
            std::cout << "testing " << t->getTestDescription() << '\n';
            t->runSanityTests();
        }
        std::cout << "done\n";

        runPerformanceTests(tests, 2 * 1024, 16 * 1000000, 2048 * 64 * 1024, 0.89, 0.33);
        auto tests2 = constructTestVector();
        runPerformanceTests(tests2, 64 * 1024, 16 * 1000000, 4 * 64 * 1024, 0.89, 0.33);

        std::cout << "The performance test results for LRUCache<size_t,size_t>\n";
        std::cout << "The first test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults1());
            std::cout << '\n';
        }
        std::cout << "The second test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time1(ms)\tSt. Dev(ms)\tAv. Time2(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests2) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults1());
            std::cout << '\n';
        }
        std::cout << "The performance test results for "
            "LRUCache<std::string,std::string>\n";
        std::cout << "Just the first 10% percent of samples are used"
            " for LRUCache<std::string,std::string>\n";
        std::cout << "The first test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults2());
            std::cout << '\n';
        }
        std::cout << "The second test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time1(ms)\tSt. Dev(ms)\tAv. Time2(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests2) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults2());
            std::cout << '\n';
        }
    }
    catch (std::exception& e) {
        std::cerr << "An exception has occurred: " << e.what() << '\n';
    }
    return 0;
}
