#include <gtest/gtest.h>
#include "data/CsvPriceSource.h"
#include "data/PythPriceSource.h"
#include "data/Aggregator.h"

TEST(CsvPriceSource, ReadsDataCorrectly) {
    CsvPriceSource source{"../../price_history/BTCUSD-1m-test.csv"};
    auto bars = source.fetch();

    ASSERT_EQ(bars.size(), 3);

    // Check first bar
    EXPECT_EQ(bars[0].timestamp, 1672531200000);
    EXPECT_DOUBLE_EQ(bars[0].open, 16546.8);
    EXPECT_DOUBLE_EQ(bars[0].high, 16547.2);
    EXPECT_DOUBLE_EQ(bars[0].low, 16546.5);
    EXPECT_DOUBLE_EQ(bars[0].close, 16547.0);
    EXPECT_DOUBLE_EQ(bars[0].volume, 100.5);

    // Check last bar
    EXPECT_EQ(bars[2].timestamp, 1672531320000);
    EXPECT_DOUBLE_EQ(bars[2].close, 16549.5);
}

TEST(CsvPriceSource, ThrowsOnMissingFile) {
    CsvPriceSource source{"nonexistent_file.csv"};
    EXPECT_THROW(source.fetch(), std::runtime_error);
}

// Friend class to access private members of PythPriceSource
class PythPriceSourceTest : public ::testing::Test {
protected:
    static std::vector<Bar> callParseJsonResponse(const std::string& json) {
        return PythPriceSource::parseJsonResponse(json);
    }
};

TEST_F(PythPriceSourceTest, ParsesValidResponse) {
    std::string mockResponse = R"({
        "s": "ok",
        "t": [1684127160, 1684127220],
        "o": [27281.83, 27297.19886],
        "h": [27296.75, 27300.03417102],
        "l": [27279.24580449, 27292.33027595],
        "c": [27296.35698548, 27294.0],
        "v": [10.5, 20.2]
    })";

    auto bars = callParseJsonResponse(mockResponse);
    ASSERT_EQ(bars.size(), 2);

    // Check first bar
    EXPECT_EQ(bars[0].timestamp, 1684127160000); // Check for ms conversion
    EXPECT_DOUBLE_EQ(bars[0].open, 27281.83);
    EXPECT_DOUBLE_EQ(bars[0].high, 27296.75);
    EXPECT_DOUBLE_EQ(bars[0].low, 27279.24580449);
    EXPECT_DOUBLE_EQ(bars[0].close, 27296.35698548);
    EXPECT_DOUBLE_EQ(bars[0].volume, 10.5);

    // Check second bar
    EXPECT_DOUBLE_EQ(bars[1].close, 27294.0);
}

TEST_F(PythPriceSourceTest, ParsesEmptyResponse) {
    std::string mockResponse = R"({"s":"ok","t":[],"o":[],"h":[],"l":[],"c":[],"v":[]})";
    auto bars = callParseJsonResponse(mockResponse);
    EXPECT_TRUE(bars.empty());
}

TEST_F(PythPriceSourceTest, ThrowsOnApiError) {
    std::string mockResponse = R"({"s":"error","errmsg":"invalid symbol"})";
    EXPECT_THROW(callParseJsonResponse(mockResponse), std::runtime_error);
}

TEST_F(PythPriceSourceTest, ThrowsOnMalformedJson) {
    std::string mockResponse = R"({"s": "ok", "t": [123, 456)";
    EXPECT_THROW(callParseJsonResponse(mockResponse), std::runtime_error);
}

TEST(Aggregator, AggregatesCorrectly) {
    // 5 one-minute bars
    std::vector<Bar> rawBars = {
        {1672531200000, 100, 110, 90, 105, 10}, // 00:00
        {1672531260000, 105, 115, 102, 112, 20}, // 00:01
        {1672531320000, 112, 120, 110, 118, 30}, // 00:02
        {1672531380000, 118, 122, 115, 120, 40}, // 00:03
        {1672531440000, 120, 125, 119, 123, 50}  // 00:04
    };

    Aggregator aggregator(5); // 5-minute resolution
    auto aggregated = aggregator.aggregate(rawBars);

    ASSERT_EQ(aggregated.size(), 1);
    
    const auto& fiveMinBar = aggregated[0];
    EXPECT_EQ(fiveMinBar.timestamp, 1672531200000); // Timestamp of the start of the 5-min window
    EXPECT_DOUBLE_EQ(fiveMinBar.open, 100);
    EXPECT_DOUBLE_EQ(fiveMinBar.high, 125);
    EXPECT_DOUBLE_EQ(fiveMinBar.low, 90);
    EXPECT_DOUBLE_EQ(fiveMinBar.close, 123);
    EXPECT_DOUBLE_EQ(fiveMinBar.volume, 10 + 20 + 30 + 40 + 50);
} 