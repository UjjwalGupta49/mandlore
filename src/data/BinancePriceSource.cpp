#include "data/BinancePriceSource.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cpr/cpr.h>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <future>
#include <chrono>

#include "engine/ThreadPool.h"

using json = nlohmann::json;

// Binance allows up to 1500 candles per request, so we calculate based on resolution
const std::map<int, long> BinancePriceSource::RESOLUTION_LIMITS = {
    {1, 1500 * 60},          // 1-min: 1500 candles = ~25 hours
    {5, 1500 * 5 * 60},      // 5-min: 1500 candles = ~5.2 days
    {15, 1500 * 15 * 60},    // 15-min: 1500 candles = ~15.6 days
    {60, 1500 * 60 * 60},    // 1-hour: 1500 candles = ~62.5 days
    {240, 1500 * 4 * 60 * 60}, // 4-hour: 1500 candles = ~250 days
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
        allBars = fetchSegmentWithRetry(from_, to_, 0);
    } else {
        // Fetch in segments in parallel
        std::cout << "Binance: Total duration exceeds limit. Fetching in parallel segments." << std::endl;
        
        // Calculate number of segments for rate limit warning
        long numSegments = (totalDuration + resolutionLimit - 1) / resolutionLimit; // Ceiling division
        if (numSegments > 10) {
            std::cout << "Binance: Warning - Large dataset (" << numSegments 
                      << " segments) may approach rate limits. Consider smaller time ranges." << std::endl;
        }
        
        // Cap concurrency to avoid overwhelming the API endpoint (Binance rate limit: 240 req/min)
        const size_t max_threads = 2;  // Reduced from 8 to avoid IP bans
        const size_t num_threads = (std::min)(max_threads, static_cast<size_t>(std::thread::hardware_concurrency()));
        ThreadPool pool(num_threads);
        
        std::vector<std::future<std::vector<Bar>>> futures;

        long currentFrom = from_;
        while (currentFrom < to_) {
            long segmentTo = std::min(currentFrom + resolutionLimit, to_);
            
            std::cout << "Binance: Queuing segment from " << currentFrom << " to " << segmentTo 
                      << " (limit=1500, 500ms delay)" << std::endl;
            
            futures.push_back(
                pool.enqueue(&BinancePriceSource::fetchSegmentWithRetry, this, currentFrom, segmentTo, 0)
            );
            
            currentFrom = segmentTo;
        }

        // Collect results from all futures
        for(size_t i = 0; i < futures.size(); ++i) {
            try {
                std::vector<Bar> segmentBars = futures[i].get();
                allBars.insert(allBars.end(), segmentBars.begin(), segmentBars.end());
            } catch (const std::exception& e) {
                std::cerr << "Binance: Segment " << i << " failed after all retry attempts: " << e.what() << ". Aborting." << std::endl;
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
    // Always add delay to respect Binance rate limits (240 req/min = ~250ms between requests)
    // Using 500ms to be conservative and avoid IP bans
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
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
                                               {"limit", "1500"}});
    
    if (r.status_code != 200) {
        std::string errorDetail = "HTTP " + std::to_string(r.status_code) + 
                                " - URL: " + r.url.str() + 
                                " - Error: " + r.error.message;
        if (!r.text.empty()) {
            errorDetail += " - Response: " + r.text;
        }
        throw std::runtime_error("Failed to fetch segment from Binance API: " + errorDetail);
    }
    
    return parseJsonResponse(r.text);
}

std::vector<Bar> BinancePriceSource::fetchSegmentWithRetry(long from, long to, int retryCount) {
    try {
        // Attempt to fetch the segment
        return fetchSegment(from, to, false);
    } catch (const std::exception& e) {
        std::string errorMsg = e.what();
        
        // Log the error with details
        std::cerr << "Binance: Fetch attempt " << (retryCount + 1) << " failed for segment [" 
                  << from << " to " << to << "]: " << errorMsg << std::endl;
        
        // Check if we should retry
        if (retryCount < MAX_RETRIES) {
            // Calculate delay with exponential backoff
            int delayMs = INITIAL_RETRY_DELAY_MS * (1 << retryCount); // 2^retryCount
            if (delayMs > MAX_RETRY_DELAY_MS) {
                delayMs = MAX_RETRY_DELAY_MS;
            }
            
            // Special handling for IP bans (HTTP 418) - use maximum delay immediately
            if (errorMsg.find("HTTP 418") != std::string::npos || 
                errorMsg.find("banned until") != std::string::npos) {
                delayMs = MAX_RETRY_DELAY_MS;
                std::cout << "Binance: IP ban detected. Using maximum delay for recovery." << std::endl;
            }
            
            std::cout << "Binance: Retrying in " << (delayMs / 1000) << " seconds (attempt " 
                      << (retryCount + 2) << "/" << (MAX_RETRIES + 1) << ")..." << std::endl;
            
            // Wait before retrying
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            
            // Recursive retry
            return fetchSegmentWithRetry(from, to, retryCount + 1);
        } else {
            // Max retries exceeded
            std::string finalError = "Binance: Max retries (" + std::to_string(MAX_RETRIES) + 
                                   ") exceeded for segment [" + std::to_string(from) + " to " + 
                                   std::to_string(to) + "]. Last error: " + errorMsg;
            throw std::runtime_error(finalError);
        }
    }
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