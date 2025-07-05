#include "data/Aggregator.h"
#include <stdexcept>

std::vector<Bar> Aggregator::aggregate(const std::vector<Bar>& raw) const {
    if (resolution_ == 0) {
        throw std::invalid_argument("Resolution cannot be zero.");
    }
    if (raw.empty()) {
        return {};
    }

    std::vector<Bar> aggregated;
    const long resolutionMillis = resolution_ * 60 * 1000;
    
    Bar currentAggBar;
    long nextTimestampBoundary = 0;

    for (const auto& bar : raw) {
        if (bar.timestamp >= nextTimestampBoundary) {
            if (currentAggBar.timestamp != 0) {
                aggregated.push_back(currentAggBar);
            }

            // Start new aggregate bar
            currentAggBar = bar;
            currentAggBar.volume = 0; // Reset volume to sum up constituent volumes
            long barTimeBucket = bar.timestamp / resolutionMillis;
            currentAggBar.timestamp = barTimeBucket * resolutionMillis;
            nextTimestampBoundary = currentAggBar.timestamp + resolutionMillis;
        }

        // Update current aggregate bar
        currentAggBar.high = std::max(currentAggBar.high, bar.high);
        currentAggBar.low = std::min(currentAggBar.low, bar.low);
        currentAggBar.close = bar.close;
        currentAggBar.volume += bar.volume;
    }

    // Add the last aggregated bar
    if (currentAggBar.timestamp != 0) {
        aggregated.push_back(currentAggBar);
    }

    return aggregated;
} 