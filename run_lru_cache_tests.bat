setlocal
for /f "delims=" %%# in ('powershell get-date -format "{yyyyMMdd_HH_mm}"') do @set timestamp=%%#

.\Release\LRUCacheTest.exe lrucache_test_results_x86_%timestamp%.txt
.\x64\Release\LRUCacheTest.exe lrucache_test_results_x64_%timestamp%.txt