# Tests CMakeLists.txt

add_executable(core_tests test_core.cpp)
target_link_libraries(core_tests PRIVATE backtest_engine GTest::gtest_main)

gtest_discover_tests(core_tests)

# Thread pool tests
add_executable(threadpool_tests test_threadpool.cpp)
target_link_libraries(threadpool_tests PRIVATE backtest_engine GTest::gtest_main)

gtest_discover_tests(threadpool_tests) 