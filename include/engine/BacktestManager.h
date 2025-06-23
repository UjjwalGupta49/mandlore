#pragma once

#include <memory>
#include "engine/ExecutionEngine.h"
#include "data/PriceSource.h"

class BacktestManager {
public:
    BacktestManager(std::shared_ptr<PriceSource> src, std::shared_ptr<IStrategy> strat)
        : priceSrc_{std::move(src)}, strategy_{std::move(strat)} {}

    void run() {
        auto raw = priceSrc_->fetch();
        ExecutionEngine engine{strategy_};
        engine.run(raw);
    }

private:
    std::shared_ptr<PriceSource> priceSrc_;
    std::shared_ptr<IStrategy> strategy_;
}; 