// LRUCacheTest.cpp : Defines the entry point for the console application.
//
//#define USE_BOOST_CONTAINERS
//#define USE_BOOST_HASH
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
    virtual const char* getTestDescription() const = 0;
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
    const char* getTestDescription() const override {
        return LRUCache<size_t, size_t>::description();
    }

    void runSanityTests() override
    {
        test1([&](size_t index)->const std::string& {return testValues[index]; });
        test1([&](size_t index) {return 2*index; });
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
    template <typename IndexToValueMapping>
    void test1(IndexToValueMapping indToValue) {
        LRUCache<std::string, 
            typename std::remove_cv<
                typename std::remove_reference<
                    decltype(indToValue(0))>::type>::type > lruCache(4);
        for (size_t i = 0; i < testKeys.size(); i++) {
            lruCache.put(testKeys[i], indToValue(i));
            auto res = lruCache.get(testKeys[i]);
            check(res != nullptr,
                "lruCache.get can't find the most recently added entry");
            if(*res != indToValue(i)) {
                std::cout << "*res = " << *res << '\n';
                std::cout << "indToValue(i) = " << indToValue(i) << '\n';
            }
            check(*res == indToValue(i),
                "1: lruCache.get returned incorrect value");
            check(lruCache.get(testKeys[0]) != nullptr,
                "lruCache.get can't find the second most recently used entry");
        }
        check(*lruCache.get(testKeys[0]) == indToValue(0),
            "2: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[1]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[2]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[3]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        check(lruCache.get(testKeys[4]) != nullptr,
            "lruCache.get can't find 4th MRU entry");
        check(*lruCache.get(testKeys[4]) == indToValue(4),
            "3: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[5]) != nullptr,
            "lruCache.get can't find 3rd MRU entry");
        check(*lruCache.get(testKeys[5]) == indToValue(5),
            "4: lruCache.get returned incorrect value");
        check(lruCache.get(testKeys[6]) != nullptr,
            "lruCache.get can't find 2nd MRU entry");
        check(*lruCache.get(testKeys[6]) == indToValue(6),
            "5: lruCache.get returned incorrect value");
        lruCache.put(testKeys[1], indToValue(1));
        check(lruCache.get(testKeys[0]) == nullptr,
            "an entry is still in cache, but it is expected to be already replaced by a more recent one");
        lruCache.put(testKeys[1], indToValue(0));
        check(lruCache.get(testKeys[1]) != nullptr,
            "lruCache.get can't find the most recently added entry");
        check(*lruCache.get(testKeys[1]) == indToValue(0),
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

#ifdef USE_BOOST_HASH
template <typename KeyType>
using HashFuncType = boost::hash<KeyType>;
#else
template <typename KeyType>
using HashFuncType = std::hash<KeyType>;
#endif


template <typename KeyType, typename  ValueType>
using LRUCache_V1 = LRUCacheV1::LRUCache<KeyType, ValueType, void, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1u = LRUCacheV1::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1a = LRUCacheV1::LRUCache<KeyType, ValueType, void, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V1ua = LRUCacheV1::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2 = LRUCacheV2::LRUCache<KeyType, ValueType, void, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2u = LRUCacheV2::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, false>;

template <typename KeyType, typename  ValueType>
using LRUCache_V2a = LRUCacheV2::LRUCache<KeyType, ValueType, void, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V2ua = LRUCacheV2::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V3 = LRUCacheV3::LRUCache<KeyType, ValueType, void>;
template <typename KeyType, typename  ValueType>
using LRUCache_V3u = LRUCacheV3::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4 = LRUCacheV4::LRUCache<KeyType, ValueType, void, false>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4u = LRUCacheV4::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, false>;

template <typename KeyType, typename  ValueType>
using LRUCache_V4a = LRUCacheV4::LRUCache<KeyType, ValueType, void, true>;
template <typename KeyType, typename  ValueType>
using LRUCache_V4ua = LRUCacheV4::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>, true>;

template <typename KeyType, typename  ValueType>
using LRUCache_V5 = LRUCacheV5::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>>;

template <typename KeyType, typename  ValueType>
using LRUCache_V6 = LRUCacheV6::LRUCache<
    KeyType, ValueType, HashFuncType<KeyType>>;

std::vector<std::unique_ptr<LRUCacheTest>> constructTestVector() {
    std::unique_ptr<LRUCacheTest> init_list[] = {
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
        std::random_shuffle(testPermutation.begin(), testPermutation.end());
        std::cout << "\nIteration #" << i;
        for (auto j : testPermutation) {
            auto& t = tests[j];
            std::cout << '.';
            //std::cout << "testing " << t->getTestDescription() << '\n';
            t->runPerformanceTest(cacheSize, testSeq.first, testSeq.second);
        }
    }
    std::cout << "\ndone\nvalidating LRUCache<size_t,size_t> test results consistency..\n";
    auto& theResultToCompareWith1 = tests.front()->getTestResults1()[0];
    for (auto& t : tests) {
        t->validateTestResults(t->getTestResults1(), theResultToCompareWith1);
    }
    std::cout << "\ndone\n";
    std::cout << "all LRUCache<size_t,size_t> tests reported the following statistic:\n";
    theResultToCompareWith1.printStatistics();

    std::cout << "validating LRUCache<std::string,std::string> test results consistency..\n";
    auto& theResultToCompareWith2 = tests.front()->getTestResults2()[0];
    for (auto& t : tests) {
        t->validateTestResults(t->getTestResults2(), theResultToCompareWith2);
    }
    std::cout << "done\n";
    std::cout << "all LRUCache<string,size_t> tests reported the following statistic:\n";
    theResultToCompareWith2.printStatistics();
}

int main() {
    try {

        std::time_t t = std::time(nullptr);
        std::cout << "local time: " << std::put_time(std::localtime(&t), "%F %T %z") << '\n'; 
#ifdef __clang__
        std::cout << "Compiler: Clang(" << __clang__ << ")\n";
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

        std::cout << "The first test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time1(ms)\tSt. Dev(ms)\tAv. Time2(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults1());
            std::cout << '\t';
            t->printTestResults(t->getTestResults2());
            std::cout << '\n';
        }
        std::cout << "The second test sequence results summary:\n";
        std::cout << "Test Name\tAv. Time1(ms)\tSt. Dev(ms)\tAv. Time2(ms)\tSt. Dev(ms)\n";
        for (auto& t : tests2) {
            std::cout << t->getTestDescription() << '\t';
            t->printTestResults(t->getTestResults1());
            std::cout << '\t';
            t->printTestResults(t->getTestResults2());
            std::cout << '\n';
        }
        std::cout << "Time1 is for LRUCache<size_t,size_t> and Time2 for LRUCache<std::string,std::string>\n";
        std::cout << "Just the first 10% percent of samples are used for LRUCache<std::string,std::string>\n";
    }
    catch (std::exception& e) {
        std::cerr << "An exception has occurred: " << e.what() << '\n';
    }
    return 0;
}
