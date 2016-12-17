echo std::unordered_map + hand made double linked list over vector compiled for x86 > lrucache_test_results.txt
.\ReleaseV1u\LRUCacheTest.exe >> lrucache_test_results.txt
echo std::unordered_map + hand made double linked list over vector compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV1u\LRUCacheTest.exe >> lrucache_test_results.txt

echo std::unordered_map + std::list compiled for x86 >> lrucache_test_results.txt
.\ReleaseV2u\LRUCacheTest.exe >> lrucache_test_results.txt
echo std::unordered_map + std::list compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV2u\LRUCacheTest.exe >> lrucache_test_results.txt

echo boost::intrusive::unordered_set + boost::intrusive::list compiled for x86 >> lrucache_test_results.txt
.\ReleaseV3u\LRUCacheTest.exe >> lrucache_test_results.txt
echo boost::intrusive::unordered_set + boost::intrusive::list compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV3u\LRUCacheTest.exe >> lrucache_test_results.txt

echo "boost::multi_index_container<indexed_by<sequenced<>,hashed_unique<...>> compiled for x86" >> lrucache_test_results.txt
.\ReleaseV4u\LRUCacheTest.exe >> lrucache_test_results.txt
echo "boost::multi_index_container<indexed_by<sequenced<>,hashed_unique<...>> compiled for x64" >> lrucache_test_results.txt
.\x64\ReleaseV4u\LRUCacheTest.exe >> lrucache_test_results.txt

echo std::map + hand made double linked list over vector compiled for x86 >> lrucache_test_results.txt
.\ReleaseV1\LRUCacheTest.exe >> lrucache_test_results.txt
echo std::map + hand made double linked list over vector compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV1\LRUCacheTest.exe >> lrucache_test_results.txt

echo std::map + std::list compiled for x86 >> lrucache_test_results.txt
.\ReleaseV2\LRUCacheTest.exe >> lrucache_test_results.txt
echo std::map + std::list compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV2\LRUCacheTest.exe >> lrucache_test_results.txt

echo boost::intrusive::set + boost::intrusive::list compiled for x86 >> lrucache_test_results.txt
.\ReleaseV3\LRUCacheTest.exe >> lrucache_test_results.txt
echo boost::intrusive::set + boost::intrusive::list compiled for x64 >> lrucache_test_results.txt
.\x64\ReleaseV3\LRUCacheTest.exe >> lrucache_test_results.txt


echo "boost::multi_index_container<indexed_by<sequenced<>,ordered_unique<...>> compiled for x86" >> lrucache_test_results.txt
.\ReleaseV4\LRUCacheTest.exe >> lrucache_test_results.txt
echo "boost::multi_index_container<indexed_by<sequenced<>,ordered_unique<...>> compiled for x64" >> lrucache_test_results.txt
.\x64\ReleaseV4\LRUCacheTest.exe >> lrucache_test_results.txt

echo "emilib::HashMap + hand made double linked list based on vector compiled for x86" >> lrucache_test_results.txt
.\ReleaseV5\LRUCacheTest.exe >> lrucache_test_results.txt
echo "emilib::HashMap + hand made double linked list based on vector compiled for x64" >> lrucache_test_results.txt
.\x64\ReleaseV5\LRUCacheTest.exe >> lrucache_test_results.txt
