#pragma once

#include "core/Bar.h"
#include <string>
#include <vector>

// Orchestrates loading of price data, using a local CSV cache
// and falling back to remote APIs if the cache is empty.
// Combines OHLC data from Pyth with volume and num_trades from Binance.
class PriceManager {
public:
    PriceManager(std::string symbol, std::string resolution, long from, long to);
    
    std::vector<Bar> loadData();

private:
    std::string symbol_;
    std::string resolution_;
    long from_;
    long to_;
    const std::string priceHistoryPath_ = "./price_history/";

    std::string getCsvPath() const;
    void cacheDataToCsv(const std::vector<Bar>& bars) const;
    
    // Combine OHLC data from Pyth with volume/trades data from Binance
    std::vector<Bar> combineData(const std::vector<Bar>& pythBars, const std::vector<Bar>& binanceBars) const;
}; 