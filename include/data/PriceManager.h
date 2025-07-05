#pragma once

#include "core/Bar.h"
#include <string>
#include <vector>

// Orchestrates loading of price data, using a local CSV cache
// and falling back to a remote API if the cache is empty.
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
}; 