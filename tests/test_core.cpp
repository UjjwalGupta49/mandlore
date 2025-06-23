#include <gtest/gtest.h>
#include "core/Bar.h"
#include "core/OrderRequest.h"

TEST(Bar, DefaultConstructible) {
    Bar bar{};
    // All numeric fields should initialize to zero
    EXPECT_EQ(bar.timestamp, 0);
    EXPECT_DOUBLE_EQ(bar.open, 0.0);
    EXPECT_DOUBLE_EQ(bar.high, 0.0);
    EXPECT_DOUBLE_EQ(bar.low, 0.0);
    EXPECT_DOUBLE_EQ(bar.close, 0.0);
    EXPECT_DOUBLE_EQ(bar.volume, 0.0);
}

TEST(OrderRequest, DefaultConstructible) {
    OrderRequest ord{};
    EXPECT_EQ(ord.side, Side::Long);
    EXPECT_DOUBLE_EQ(ord.sizeUsd, 0.0);
    EXPECT_DOUBLE_EQ(ord.leverage, 1.0);
} 