#include "data/BinancePriceSource.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cpr/cpr.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <future>

#include "engine/ThreadPool.h"

using json = nlohmann::json;

// Binance allows up to 1000 candles per request, so we calculate based on resolution
const std::map<int, long> BinancePriceSource::RESOLUTION_LIMITS = {
    {1, 1000 * 60},          // 1-min: 1000 candles = ~16.7 hours
    {5, 1000 * 5 * 60},      // 5-min: 1000 candles = ~3.5 days
    {15, 1000 * 15 * 60},    // 15-min: 1000 candles = ~10.4 days
    {60, 1000 * 60 * 60},    // 1-hour: 1000 candles = ~41.7 days
    {240, 1000 * 4 * 60 * 60}, // 4-hour: 1000 candles = ~166 days
};
const long ONE_YEAR_SECONDS = 365 * 24 * 60 * 60;

BinancePriceSource::BinancePriceSource(std::string symbol, std::string resolution, long from, long to)
    : symbol_{std::move(symbol)},
      resolution_{std::move(resolution)},
      from_{from},
      to_{to} {}

std::vector<Bar> BinancePriceSource::fetch() {
    long totalDuration = to_ - from_;
    int resolutionInt = 0;
    try {
        resolutionInt = std::stoi(resolution_);
    } catch (const std::invalid_argument&) {
        throw std::runtime_error("Invalid numeric resolution: " + resolution_);
    }

    long resolutionLimit = ONE_YEAR_SECONDS;
    if (RESOLUTION_LIMITS.count(resolutionInt)) {
        resolutionLimit = RESOLUTION_LIMITS.at(resolutionInt);
    }

    std::vector<Bar> allBars;

    if (totalDuration <= resolutionLimit) {
        // Fetch in a single request
        std::cout << "Binance: Total duration within limit. Fetching in a single request." << std::endl;
        allBars = fetchSegment(from_, to_, false);
    } else {
        // Fetch in segments in parallel
        std::cout << "Binance: Total duration exceeds limit. Fetching in parallel segments." << std::endl;
        
        // Cap concurrency to avoid overwhelming the API endpoint
        const size_t max_threads = 8;
        const size_t num_threads = (std::min)(max_threads, static_cast<size_t>(std::thread::hardware_concurrency()));
        ThreadPool pool(num_threads);
        
        std::vector<std::future<std::vector<Bar>>> futures;

        long currentFrom = from_;
        while (currentFrom < to_) {
            long segmentTo = std::min(currentFrom + resolutionLimit, to_);
            
            std::cout << "Binance: Queuing segment from " << currentFrom << " to " << segmentTo << std::endl;
            
            futures.push_back(
                pool.enqueue(&BinancePriceSource::fetchSegment, this, currentFrom, segmentTo, false)
            );
            
            currentFrom = segmentTo;
        }

        // Collect results from all futures
        for(auto& future : futures) {
            try {
                std::vector<Bar> segmentBars = future.get();
                allBars.insert(allBars.end(), segmentBars.begin(), segmentBars.end());
            } catch (const std::exception& e) {
                std::cerr << "Binance: A segment failed to fetch: " << e.what() << ". Aborting." << std::endl;
                throw; 
            }
        }
    }
    
    std::cout << "Binance: Successfully fetched a total of " << allBars.size() << " bars." << std::endl;

    // Sort and deduplicate
    std::sort(allBars.begin(), allBars.end(), [](const Bar& a, const Bar& b){
        return a.timestamp < b.timestamp;
    });
    
    auto last = std::unique(allBars.begin(), allBars.end(), [](const Bar& a, const Bar& b){
        return a.timestamp == b.timestamp;
    });
    allBars.erase(last, allBars.end());

    return allBars;
}

std::vector<Bar> BinancePriceSource::fetchSegment(long from, long to, bool addDelay) {
    if (addDelay) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::string binanceSymbol = convertSymbolToBinance(symbol_);
    std::string binanceInterval = convertResolutionToBinance(resolution_);

    // Binance expects timestamps in milliseconds
    long fromMs = from * 1000;
    long toMs = to * 1000;

    cpr::Response r = cpr::Get(cpr::Url{baseUrl_ + "/fapi/v1/klines"},
                               cpr::Parameters{{"symbol", binanceSymbol},
                                               {"interval", binanceInterval},
                                               {"startTime", std::to_string(fromMs)},
                                               {"endTime", std::to_string(toMs)},
                                               {"limit", "1000"}});
    
    if (r.status_code != 200) {
        throw std::runtime_error("Failed to fetch segment from Binance API: " + r.error.message);
    }
    
    return parseJsonResponse(r.text);
}

std::vector<Bar> BinancePriceSource::parseJsonResponse(const std::string& jsonBody) {
    std::vector<Bar> bars;
    try {
        json data = json::parse(jsonBody);

        if (!data.is_array()) {
            throw std::runtime_error("Binance API response is not an array");
        }

        for (const auto& kline : data) {
            if (!kline.is_array() || kline.size() < 9) {
                throw std::runtime_error("Invalid kline format in Binance API response");
            }

            Bar bar;
            // Binance format: [openTime, open, high, low, close, volume, closeTime, quoteVolume, count, ...]
            bar.timestamp = kline[0].get<long>();  // Already in milliseconds
            bar.open = std::stod(kline[1].get<std::string>());
            bar.high = std::stod(kline[2].get<std::string>());
            bar.low = std::stod(kline[3].get<std::string>());
            bar.close = std::stod(kline[4].get<std::string>());
            bar.volume = std::stod(kline[5].get<std::string>());
            bar.num_trades = kline[8].get<long>();  // Number of trades
            bars.push_back(bar);
        }

    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse Binance API JSON response: " + std::string(e.what()));
    } catch (const json::exception& e) {
        throw std::runtime_error("Missing expected field in Binance API JSON response: " + std::string(e.what()));
    }

    return bars;
}

std::string BinancePriceSource::convertSymbolToBinance(const std::string& pythSymbol) const {
    // Convert "Crypto.SOL/USD" to "SOLUSDT"
    std::string result = pythSymbol;
    
    // Remove "Crypto." prefix if present
    if (result.find("Crypto.") == 0) {
        result = result.substr(7);
    }
    
    // Replace "/" with ""
    size_t slashPos = result.find('/');
    if (slashPos != std::string::npos) {
        result.erase(slashPos, 1);
    }
    
    // Convert USD to USDT for Binance futures
    if (result.length() >= 3 && result.substr(result.length() - 3) == "USD") {
        result = result.substr(0, result.length() - 3) + "USDT";
    }
    
    return result;
}

std::string BinancePriceSource::convertResolutionToBinance(const std::string& resolution) const {
    // Convert minute resolution to Binance interval format
    int resolutionInt = std::stoi(resolution);
    
    if (resolutionInt == 1) return "1m";
    if (resolutionInt == 5) return "5m";
    if (resolutionInt == 15) return "15m";
    if (resolutionInt == 60) return "1h";
    if (resolutionInt == 240) return "4h";
    if (resolutionInt == 1440) return "1d";
    
    // Default to 1m for unsupported resolutions
    return "1m";
} 