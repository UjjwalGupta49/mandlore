#include "data/PriceManager.h"
#include "data/CsvPriceSource.h"
#include "data/PythPriceSource.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

// Helper to check for file existence
inline bool fileExists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

PriceManager::PriceManager(std::string symbol, std::string resolution, long from, long to)
    : symbol_{std::move(symbol)},
      resolution_{std::move(resolution)},
      from_{from},
      to_{to} {}

std::vector<Bar> PriceManager::loadData() {
    std::string csvPath = getCsvPath();
    if (fileExists(csvPath)) {
        std::cout << "Loading data from cached CSV: " << csvPath << std::endl;
        CsvPriceSource csvSource(csvPath);
        return csvSource.fetch();
    }
    
    std::cout << "No local cache found. Fetching from Pyth API..." << std::endl;
    PythPriceSource pythSource(symbol_, resolution_, from_, to_);
    std::vector<Bar> bars = pythSource.fetch();
    
    if (!bars.empty()) {
        std::cout << "Caching " << bars.size() << " bars to " << csvPath << "..." << std::endl;
        cacheDataToCsv(bars);
    }
    
    return bars;
}

std::string PriceManager::getCsvPath() const {
    // Create a filesystem-friendly name, e.g., "Crypto.BTC/USD" -> "Crypto_BTC_USD"
    std::string sanitizedSymbol = symbol_;
    std::replace(sanitizedSymbol.begin(), sanitizedSymbol.end(), '/', '_');
    std::replace(sanitizedSymbol.begin(), sanitizedSymbol.end(), '.', '_');
    
    // New format: ticker_resolution_startTimestamp_endTimestamp.csv
    return priceHistoryPath_ + sanitizedSymbol + "_" + resolution_ + "_" + std::to_string(from_) + "_" + std::to_string(to_) + ".csv";
}

void PriceManager::cacheDataToCsv(const std::vector<Bar>& bars) const {
    std::string csvPath = getCsvPath();
    std::ofstream file(csvPath);
    if (!file.is_open()) {
        // Not a fatal error, just log it.
        std::cerr << "Warning: Could not open file to cache data: " << csvPath << std::endl;
        return;
    }

    file << "timestamp,open,high,low,close,volume\n";
    file << std::fixed; // Use fixed-point notation for doubles
    for (const auto& bar : bars) {
        file << bar.timestamp << ","
             << bar.open << ","
             << bar.high << ","
             << bar.low << ","
             << bar.close << ","
             << bar.volume << "\n";
    }
} 