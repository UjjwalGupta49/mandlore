#include "sma_cross/SmaCrossStrategy.h"
#include <iostream>
#include <numeric>
#include <vector>

SmaCrossStrategy::SmaCrossStrategy(const StrategyConfig& config) : config_{config} {}

void SmaCrossStrategy::on_start(const Bar&, double) {
    std::cout << "SmaCrossStrategy started with capital: " << config_.initialCapital << std::endl;
}

std::vector<OrderRequest> SmaCrossStrategy::on_bar(const Bar& currentBar,
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

    // --- Crossover Logic ---
    if (!isInitialized_) {
        wasPriceAboveSma_ = currentBar.close > currentSma_;
        isInitialized_ = true;
        return {};
    }

    bool isPriceAboveSma = currentBar.close > currentSma_;
    bool crossUp = !wasPriceAboveSma_ && isPriceAboveSma;
    bool crossDown = wasPriceAboveSma_ && !isPriceAboveSma;
    wasPriceAboveSma_ = isPriceAboveSma;

    std::vector<OrderRequest> orders;

    if (!openPositions.empty()) {
        Side currentSide = openPositions[0].side;
        if (currentSide == Side::Long && crossDown) {
            orders.push_back({.side = Side::Short});
            std::cout << "STRAT: Price crossed below SMA. Signaling to close LONG." << std::endl;
        } else if (currentSide == Side::Short && crossUp) {
            orders.push_back({.side = Side::Long});
            std::cout << "STRAT: Price crossed above SMA. Signaling to close SHORT." << std::endl;
        }
    } else { // No position is open
        if (crossUp) {
            OrderRequest order{.side = Side::Long, .sizeUsd = config_.perTradeSize};
            orders.push_back(order);
            std::cout << "STRAT: Price crossed above SMA. Signaling to open LONG." << std::endl;
        } else if (crossDown) {
            OrderRequest order{.side = Side::Short, .sizeUsd = config_.perTradeSize};
            orders.push_back(order);
            std::cout << "STRAT: Price crossed below SMA. Signaling to open SHORT." << std::endl;
        }
    }

    return orders;
}

void SmaCrossStrategy::on_finish() {
    std::cout << "SmaCrossStrategy finished." << std::endl;
}

const StrategyConfig& SmaCrossStrategy::getConfig() const {
    return config_;
} 