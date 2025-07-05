#include "buy_and_hold/BuyAndHoldStrategy.h"
#include <iostream>

void BuyAndHoldStrategy::on_start(const Bar&, double) {
    std::cout << "BuyAndHoldStrategy: Starting." << std::endl;
}

std::vector<OrderRequest> BuyAndHoldStrategy::on_bar(const Bar&, const std::vector<Position>& positions, double) {
    if (!alreadyBought_ && positions.empty()) {
        std::cout << "BuyAndHoldStrategy: Issuing buy order." << std::endl;
        alreadyBought_ = true;
        OrderRequest buyOrder;
        buyOrder.side = Side::Long;
        buyOrder.sizeUsd = 10000; // Buy $10,000 worth
        buyOrder.leverage = 1.0;
        return {buyOrder};
    }
    return {}; // No orders on subsequent bars
}

void BuyAndHoldStrategy::on_finish() {
    std::cout << "BuyAndHoldStrategy: Finished." << std::endl;
} 