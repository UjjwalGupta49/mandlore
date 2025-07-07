#include "sma_cross/SmaCrossStrategy.h"
#include <iostream>
#include <numeric>
#include <vector>

SmaCrossStrategy::SmaCrossStrategy(const StrategyConfig& config) : config_{config} {}

void SmaCrossStrategy::on_start(const Bar&, double) {
    std::cout << "SmaCrossStrategy started with capital: " << config_.initialCapital << std::endl;
}

StrategyAction SmaCrossStrategy::on_bar(const Bar& currentBar,
                                                     const std::vector<Position>& openPositions,
                                                     double) {
    priceHistory_.push_back(currentBar.close);
    if (priceHistory_.size() > smaPeriod_) {
        priceHistory_.pop_front();
    }

    if (priceHistory_.size() < smaPeriod_) {
        return {}; // Not enough data yet
    }
    
    double sma = std::accumulate(priceHistory_.begin(), priceHistory_.end(), 0.0) / smaPeriod_;

    smaHistory_.push_back(sma);
    if (smaHistory_.size() > smoothingPeriod_) {
        smaHistory_.pop_front();
    }

    if (smaHistory_.size() < smoothingPeriod_) {
        return {};
    }
    currentSma_ = std::accumulate(smaHistory_.begin(), smaHistory_.end(), 0.0) / smoothingPeriod_;

    // --- Threshold Logic ---
    const double upperBand = currentSma_ * 1.02;
    const double lowerBand = currentSma_ * 0.98;

    StrategyAction action;

    if (!openPositions.empty()) {
        Side currentSide = openPositions[0].side;
        if (currentSide == Side::Long && currentBar.close < currentSma_) {
            action.closeCurrentPosition = true;
            std::cout << "STRAT: Price is below SMA. Signaling to CLOSE LONG." << std::endl;
        } else if (currentSide == Side::Short && currentBar.close > currentSma_) {
            action.closeCurrentPosition = true;
            std::cout << "STRAT: Price is above SMA. Signaling to CLOSE SHORT." << std::endl;
        }
    } else { // No position is open
        if (currentBar.close > upperBand) {
            action.openRequests.push_back({.side = Side::Long, .sizeUsd = config_.perTradeSize});
            std::cout << "STRAT: Price is 2% above SMA. Signaling to OPEN LONG." << std::endl;
        } else if (currentBar.close < lowerBand) {
            action.openRequests.push_back({.side = Side::Short, .sizeUsd = config_.perTradeSize});
            std::cout << "STRAT: Price is 2% below SMA. Signaling to OPEN SHORT." << std::endl;
        }
    }

    return action;
}

void SmaCrossStrategy::on_finish() {
    std::cout << "SmaCrossStrategy finished." << std::endl;
}

const StrategyConfig& SmaCrossStrategy::getConfig() const {
    return config_;
} 