#include "buy_and_hold/BuyAndHoldStrategy.h"
#include <iostream>
#include <vector>

BuyAndHoldStrategy::BuyAndHoldStrategy(const StrategyConfig& config) : config_{config} {}

void BuyAndHoldStrategy::on_start(const Bar&, double) {
    std::cout << "BuyAndHoldStrategy started with capital: " << config_.initialCapital << std::endl;
    std::cout << "Stop loss: " << config_.stopLossPercent << "%, Take profit: " << config_.takeProfitPercent << "%" << std::endl;
}

std::vector<OrderRequest> BuyAndHoldStrategy::on_bar(const Bar& currentBar,
                                                         const std::vector<Position>& openPositions,
                                                         double) {
    std::vector<OrderRequest> orders;
    if (!invested_ && openPositions.empty()) {
        OrderRequest order;
        order.side = Side::Long;
        order.sizeUsd = config_.perTradeSize;

        // Set SL/TP based on config
        const double entryPrice = currentBar.close;
        if (config_.stopLossPercent > 0) {
            order.stopLossPrice = entryPrice * (1.0 - (config_.stopLossPercent / 100.0));
        }
        if (config_.takeProfitPercent > 0) {
            order.takeProfitPrice = entryPrice * (1.0 + (config_.takeProfitPercent / 100.0));
        }

        orders.push_back(order);
        invested_ = true;
        std::cout << "STRAT: Placing buy order for " << order.sizeUsd << " USD @ " << entryPrice 
                  << " SL: " << order.stopLossPrice << " TP: " << order.takeProfitPrice << std::endl;
    }
    return orders;
}

void BuyAndHoldStrategy::on_finish() {
    std::cout << "BuyAndHoldStrategy finished." << std::endl;
}

const StrategyConfig& BuyAndHoldStrategy::getConfig() const {
    return config_;
} 