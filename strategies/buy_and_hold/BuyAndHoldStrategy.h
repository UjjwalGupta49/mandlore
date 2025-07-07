#pragma once

#include "strategy/IStrategy.h"
#include "strategy/StrategyConfig.h"

// A simple strategy that buys on the first bar and holds until the end.
class BuyAndHoldStrategy : public IStrategy {
public:
    explicit BuyAndHoldStrategy(const StrategyConfig& config);

    void on_start(const Bar& firstBar, double initialEquity) override;

    StrategyAction on_bar(const Bar& currentBar,
                          const std::vector<Position>& openPositions,
                          double accountEquity) override;

    void on_finish() override;

    const StrategyConfig& getConfig() const override;

private:
    bool invested_{false};
    StrategyConfig config_;
}; 