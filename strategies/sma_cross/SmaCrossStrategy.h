#pragma once

#include "strategy/IStrategy.h"
#include "strategy/StrategyConfig.h"
#include <vector>
#include <deque>
#include <cstddef>

class SmaCrossStrategy : public IStrategy {
public:
    explicit SmaCrossStrategy(const StrategyConfig& config);

    void on_start(const Bar& firstBar, double initialEquity) override;

    std::vector<OrderRequest> on_bar(const Bar& currentBar,
                                     const std::vector<Position>& openPositions,
                                     double accountEquity) override;

    void on_finish() override;

    const StrategyConfig& getConfig() const override;

private:
    void updateSma(double price);

    StrategyConfig config_;
    const std::size_t smaPeriod_{20};
    const std::size_t smoothingPeriod_{14};

    std::deque<double> priceHistory_;
    std::deque<double> smaHistory_;
    double currentSma_{0.0};
    
    bool wasPriceAboveSma_{false};
    bool isInitialized_{false};
}; 