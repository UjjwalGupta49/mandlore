# Tests CMakeLists.txt
include(GoogleTest)

add_executable(core_tests test_core.cpp)
target_compile_options(core_tests PRIVATE ${MY_PROJECT_COMPILE_OPTIONS})
target_link_libraries(core_tests PRIVATE backtest_engine GTest::gtest_main)

gtest_discover_tests(core_tests)

# Thread pool tests
add_executable(threadpool_tests test_threadpool.cpp)
target_compile_options(threadpool_tests PRIVATE ${MY_PROJECT_COMPILE_OPTIONS})
target_link_libraries(threadpool_tests PRIVATE backtest_engine GTest::gtest_main)

gtest_discover_tests(threadpool_tests)

# Data layer tests
add_executable(data_tests test_data.cpp)
target_compile_options(data_tests PRIVATE ${MY_PROJECT_COMPILE_OPTIONS})
target_link_libraries(data_tests PRIVATE backtest_engine GTest::gtest_main)

gtest_discover_tests(data_tests) 