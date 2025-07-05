#pragma once

#include <string>

struct StrategyConfig {
    double initialCapital{10000.0};
    double stopLossPercent{0.0};
    double takeProfitPercent{0.0};
    double perTradeSize{0.0};
}; 