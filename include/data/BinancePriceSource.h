#pragma once

#include "data/PriceSource.h"
#include <string>
#include <map>

// Fetches price data from the Binance Futures API for volume and trades data.
class BinancePriceSource : public PriceSource {
public:
    BinancePriceSource(std::string symbol, std::string resolution, long from, long to);

    std::vector<Bar> fetch() override;

private:
    std::string symbol_;
    std::string resolution_;
    long from_;
    long to_;
    const std::string baseUrl_ = "https://fapi.binance.com";

    // Fetches a single segment of data, respecting rate limits.
    std::vector<Bar> fetchSegment(long from, long to, bool addDelay);

    // Helper to facilitate testing of the parsing logic
    static std::vector<Bar> parseJsonResponse(const std::string& jsonBody);

    // Convert symbol format from Pyth to Binance (e.g., "Crypto.SOL/USD" -> "SOLUSDT")
    std::string convertSymbolToBinance(const std::string& pythSymbol) const;

    // Convert resolution from minutes to Binance interval format
    std::string convertResolutionToBinance(const std::string& resolution) const;

    // Resolution-based chunking limits (in seconds) - Binance allows up to 1000 candles per request
    static const std::map<int, long> RESOLUTION_LIMITS;
    
    // Retry configuration - increased delays to handle IP bans
    static const int MAX_RETRIES = 3;
    static const int INITIAL_RETRY_DELAY_MS = 30000; // 30 seconds for IP ban recovery
    static const int MAX_RETRY_DELAY_MS = 300000; // 5 minutes maximum delay
    
    // Helper method for retrying failed requests
    std::vector<Bar> fetchSegmentWithRetry(long from, long to, int retryCount = 0);

    // Give test class access to private members
    friend class BinancePriceSourceTest;
}; 