#include "data/PriceManager.h"
#include "data/CsvPriceSource.h"
#include "data/PythPriceSource.h"
#include "data/BinancePriceSource.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>
#include <system_error>
#include <unordered_map>

// Helper to check for file existence
inline bool fileExists(const std::string& name) {
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

// Helper to create directory if it doesn't exist
inline bool createDirectoryIfNotExists(const std::string& path) {
    if (fileExists(path)) {
        return true;  // Directory already exists
    }
    
    #ifdef _WIN32
    int result = mkdir(path.c_str());
    #else
    int result = mkdir(path.c_str(), 0755);  // rwxr-xr-x permissions
    #endif
    
    if (result == 0) {
        std::cout << "Created directory: " << path << std::endl;
        return true;
    }
    
    // Check if another process created the directory in the meantime
    if (fileExists(path)) {
        return true;
    }
    
    std::error_code ec;
    std::string errorMsg = std::system_category().message(errno);
    std::cerr << "Failed to create directory " << path << ": " << errorMsg << std::endl;
    return false;
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
    
    std::cout << "No local cache found. Fetching from Pyth and Binance APIs..." << std::endl;
    
    // Fetch OHLC data from Pyth
    std::cout << "Fetching OHLC data from Pyth..." << std::endl;
    PythPriceSource pythSource(symbol_, resolution_, from_, to_);
    std::vector<Bar> pythBars = pythSource.fetch();
    
    // Fetch volume and trades data from Binance
    std::cout << "Fetching volume and trades data from Binance..." << std::endl;
    BinancePriceSource binanceSource(symbol_, resolution_, from_, to_);
    std::vector<Bar> binanceBars = binanceSource.fetch();
    
    // Combine the data sources
    std::cout << "Combining data from both sources..." << std::endl;
    std::vector<Bar> combinedBars = combineData(pythBars, binanceBars);
    
    if (!combinedBars.empty()) {
        std::cout << "Caching " << combinedBars.size() << " combined bars to " << csvPath << "..." << std::endl;
        cacheDataToCsv(combinedBars);
    }
    
    return combinedBars;
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
    // Ensure the price_history directory exists
    if (!createDirectoryIfNotExists(priceHistoryPath_)) {
        std::cerr << "Warning: Could not create price history directory. Data will not be cached." << std::endl;
        return;
    }

    std::string csvPath = getCsvPath();
    std::ofstream file(csvPath);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open file to cache data: " << csvPath << std::endl;
        return;
    }

    file << "timestamp,open,high,low,close,volume,num_trades\n";
    file << std::fixed; // Use fixed-point notation for doubles
    for (const auto& bar : bars) {
        file << bar.timestamp << ","
             << bar.open << ","
             << bar.high << ","
             << bar.low << ","
             << bar.close << ","
             << bar.volume << ","
             << bar.num_trades << "\n";
    }
}

std::vector<Bar> PriceManager::combineData(const std::vector<Bar>& pythBars, const std::vector<Bar>& binanceBars) const {
    std::vector<Bar> combinedBars;
    
    // Create a hash map of Binance data by timestamp for quick lookup
    std::unordered_map<long, const Bar*> binanceDataMap;
    for (const auto& bar : binanceBars) {
        binanceDataMap[bar.timestamp] = &bar;
    }
    
    // Iterate through Pyth data and combine with Binance data
    for (const auto& pythBar : pythBars) {
        Bar combinedBar;
        
        // Use OHLC data from Pyth (more accurate pricing)
        combinedBar.timestamp = pythBar.timestamp;
        combinedBar.open = pythBar.open;
        combinedBar.high = pythBar.high;
        combinedBar.low = pythBar.low;
        combinedBar.close = pythBar.close;
        
        // Try to get volume and trades data from Binance
        auto binanceIt = binanceDataMap.find(pythBar.timestamp);
        if (binanceIt != binanceDataMap.end()) {
            combinedBar.volume = binanceIt->second->volume;
            combinedBar.num_trades = binanceIt->second->num_trades;
        } else {
            // Fallback to Pyth data if Binance data not available (will be 0 for volume)
            combinedBar.volume = pythBar.volume;
            combinedBar.num_trades = pythBar.num_trades;
            
            if (pythBar.volume == 0.0) {
                // Log missing Binance data for debugging
                std::cout << "Warning: No Binance data found for timestamp " << pythBar.timestamp << std::endl;
            }
        }
        
        combinedBars.push_back(combinedBar);
    }
    
    // Add any Binance bars that don't have corresponding Pyth data
    std::unordered_map<long, bool> pythTimestamps;
    for (const auto& bar : pythBars) {
        pythTimestamps[bar.timestamp] = true;
    }
    
    for (const auto& binanceBar : binanceBars) {
        if (pythTimestamps.find(binanceBar.timestamp) == pythTimestamps.end()) {
            // This Binance bar doesn't have a corresponding Pyth bar
            // Use Binance data for all fields
            combinedBars.push_back(binanceBar);
            std::cout << "Warning: Using Binance OHLC data for timestamp " << binanceBar.timestamp << " (no Pyth data)" << std::endl;
        }
    }
    
    // Sort by timestamp to ensure proper ordering
    std::sort(combinedBars.begin(), combinedBars.end(), [](const Bar& a, const Bar& b) {
        return a.timestamp < b.timestamp;
    });
    
    std::cout << "Successfully combined " << pythBars.size() << " Pyth bars with " 
              << binanceBars.size() << " Binance bars into " << combinedBars.size() << " combined bars." << std::endl;
    
    return combinedBars;
} 