#include <gtest/gtest.h>
#include "engine/ThreadPool.h"
 
TEST(ThreadPool, ExecutesTask) {
    ThreadPool pool{1};
    auto future = pool.enqueue([] { return 42; });
    EXPECT_EQ(future.get(), 42);
} 