clang++ -pthread -D_LIBCPP_STD_VER=17 -O3 -std=c++14 -I. -I /usr/local/boost-1.63.0/include -L/usr/local/boost-1.63.0/lib -lboost_container LRUCacheTest.cpp -o LRUCacheTest_clang
times
