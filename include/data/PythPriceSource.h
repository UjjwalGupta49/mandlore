#pragma once

#include "data/PriceSource.h"
#include <string>
#include <map>

// Fetches price data from the Pyth network benchmarks API.
class PythPriceSource : public PriceSource {
public:
    PythPriceSource(std::string symbol, std::string resolution, long from, long to);

    std::vector<Bar> fetch() override;

private:
    std::string symbol_;
    std::string resolution_;
    long from_;
    long to_;
    const std::string baseUrl_ = "https://benchmarks.pyth.network";

    // Fetches a single segment of data, respecting rate limits.
    std::vector<Bar> fetchSegment(long from, long to, bool addDelay);

    // Helper to facilitate testing of the parsing logic
    static std::vector<Bar> parseJsonResponse(const std::string& jsonBody);

    // Resolution-based chunking limits (in seconds)
    static const std::map<int, long> RESOLUTION_LIMITS;

    // Give test class access to private members
    friend class PythPriceSourceTest;
}; 