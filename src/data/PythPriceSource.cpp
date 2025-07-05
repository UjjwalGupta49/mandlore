#include "data/PythPriceSource.h"
#include "nlohmann/json.hpp"
#include <algorithm>
#include <cpr/cpr.h>
#include <iostream>
#include <stdexcept>
#include <thread> // for std::this_thread::sleep_for
#include <future>

#include "engine/ThreadPool.h"

using json = nlohmann::json;

// Based on reference TS code: max request duration in seconds per resolution
const std::map<int, long> PythPriceSource::RESOLUTION_LIMITS = {
    {1, 2 * 24 * 60 * 60},      // 1-min: 2 days
    {5, 15 * 24 * 60 * 60},     // 5-min: 15 days
    {15, 45 * 24 * 60 * 60},    // 15-min: 45 days
    {60, 180 * 24 * 60 * 60},   // 1-hour: 180 days
    {240, 720 * 24 * 60 * 60}, // 4-hour: 720 days
};
const long ONE_YEAR_SECONDS = 365 * 24 * 60 * 60;

PythPriceSource::PythPriceSource(std::string symbol, std::string resolution, long from, long to)
    : symbol_{std::move(symbol)},
      resolution_{std::move(resolution)},
      from_{from},
      to_{to} {}

std::vector<Bar> PythPriceSource::fetch() {
    long totalDuration = to_ - from_;
    int resolutionInt = 0;
    try {
        resolutionInt = std::stoi(resolution_);
    } catch (const std::invalid_argument&) {
        // Handle non-numeric resolutions like 'D', 'W', 'M' if necessary
        // For now, we assume numeric minute-based resolutions.
        throw std::runtime_error("Invalid numeric resolution: " + resolution_);
    }

    long resolutionLimit = ONE_YEAR_SECONDS;
    if (RESOLUTION_LIMITS.count(resolutionInt)) {
        resolutionLimit = RESOLUTION_LIMITS.at(resolutionInt);
    }

    std::vector<Bar> allBars;

    if (totalDuration <= resolutionLimit) {
        // Fetch in a single request
        std::cout << "Total duration within limit. Fetching in a single request." << std::endl;
        allBars = fetchSegment(from_, to_, false);
    } else {
        // Fetch in segments in parallel
        std::cout << "Total duration exceeds limit. Fetching in parallel segments." << std::endl;
        
        // Cap concurrency to avoid overwhelming the API endpoint.
        const size_t max_threads = 8;
        const size_t num_threads = (std::min)(max_threads, static_cast<size_t>(std::thread::hardware_concurrency()));
        ThreadPool pool(num_threads);
        
        std::vector<std::future<std::vector<Bar>>> futures;

        long currentFrom = from_;
        while (currentFrom < to_) {
            long segmentTo = std::min(currentFrom + resolutionLimit, to_);
            
            std::cout << "Queuing segment from " << currentFrom << " to " << segmentTo << std::endl;
            
            // The pool size provides inherent rate limiting. No artificial delay needed.
            futures.push_back(
                pool.enqueue(&PythPriceSource::fetchSegment, this, currentFrom, segmentTo, false)
            );
            
            currentFrom = segmentTo;
        }

        // Collect results from all futures
        for(auto& future : futures) {
            try {
                std::vector<Bar> segmentBars = future.get();
                allBars.insert(allBars.end(), segmentBars.begin(), segmentBars.end());
            } catch (const std::exception& e) {
                std::cerr << "A segment failed to fetch: " << e.what() << ". Aborting." << std::endl;
                throw; 
            }
        }
    }
    
    std::cout << "Successfully fetched a total of " << allBars.size() << " bars from Pyth." << std::endl;

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

std::vector<Bar> PythPriceSource::fetchSegment(long from, long to, bool addDelay) {
    if (addDelay) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    cpr::Response r = cpr::Get(cpr::Url{baseUrl_ + "/v1/shims/tradingview/history"},
                               cpr::Parameters{{"symbol", symbol_},
                                               {"resolution", resolution_},
                                               {"from", std::to_string(from)},
                                               {"to", std::to_string(to)}});
    
    if (r.status_code != 200) {
        throw std::runtime_error("Failed to fetch segment from Pyth API: " + r.error.message);
    }
    
    // Check for "no_data" status, which is not a failure for a segment
    if (r.text.find("\"s\":\"no_data\"") != std::string::npos) {
        std::cout << "Pyth returned 'no_data' for segment." << std::endl;
        return {}; // Return empty vector
    }
    
    return parseJsonResponse(r.text);
}

std::vector<Bar> PythPriceSource::parseJsonResponse(const std::string& jsonBody) {
    std::vector<Bar> bars;
    try {
        json data = json::parse(jsonBody);

        if (data.value("s", "") != "ok") {
            // "no_data" is now handled before calling this function, 
            // but we keep this check for other errors.
            throw std::runtime_error("Pyth API returned an error status: " + data.dump());
        }
        
        // Handle case where status is "ok" but there are no timestamps
        if (!data.contains("t") || data.at("t").empty()) {
            return bars;
        }

        const auto& timestamps = data.at("t");
        const auto& opens = data.at("o");
        const auto& highs = data.at("h");
        const auto& lows = data.at("l");
        const auto& closes = data.at("c");
        const auto& volumes = data.at("v");

        if (timestamps.size() != opens.size() || timestamps.size() != highs.size()) {
            throw std::runtime_error("Mismatched array sizes in Pyth API response.");
        }

        for (size_t i = 0; i < timestamps.size(); ++i) {
            Bar bar;
            // Pyth timestamp is in seconds, Bar struct expects milliseconds
            bar.timestamp = timestamps[i].get<long>() * 1000;
            bar.open = opens[i].get<double>();
            bar.high = highs[i].get<double>();
            bar.low = lows[i].get<double>();
            bar.close = closes[i].get<double>();
            bar.volume = volumes[i].get<double>();
            bars.push_back(bar);
        }

    } catch (const json::parse_error& e) {
        throw std::runtime_error("Failed to parse Pyth API JSON response: " + std::string(e.what()));
    } catch (const json::exception& e) {
        throw std::runtime_error("Missing expected field in Pyth API JSON response: " + std::string(e.what()));
    }

    return bars;
} 