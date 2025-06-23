#pragma once

#include <memory>
#include <vector>
#include "core/Bar.h"
#include "core/Position.h"
#include "core/Trade.h"
#include "strategy/IStrategy.h"
#include "engine/ThreadPool.h"

class ExecutionEngine {
public:
    explicit ExecutionEngine(std::shared_ptr<IStrategy> strategy)
        : strategy_{std::move(strategy)}, threadPool_{1} {}

    void run(const std::vector<Bar>& bars) {
        if (bars.empty()) return;
        strategy_->on_start(bars.front(), /*initialEquity=*/10000.0);
        for (const auto& bar : bars) {
            auto fut = threadPool_.enqueue([this, &bar] {
                return strategy_->on_bar(bar, positions_, accountEquity_);
            });
            auto orders = fut.get();
            // TODO: apply orders to positions_, update trades_ and accountEquity_
        }
        strategy_->on_finish();
    }

private:
    std::shared_ptr<IStrategy> strategy_;
    ThreadPool threadPool_;

    std::vector<Position> positions_{};
    std::vector<Trade> trades_{};
    double accountEquity_{10000.0};
}; 