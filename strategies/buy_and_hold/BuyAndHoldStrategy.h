#pragma once

#include "strategy/IStrategy.h"

// A simple strategy that buys on the first bar and holds until the end.
class BuyAndHoldStrategy : public IStrategy {
public:
    void on_start(const Bar&, double) override;

    std::vector<OrderRequest> on_bar(const Bar&, const std::vector<Position>&, double) override;
    
    void on_finish() override;

private:
    bool alreadyBought_ = false;
}; 