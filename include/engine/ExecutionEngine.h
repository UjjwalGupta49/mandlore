#pragma once

#include <memory>
#include <vector>
#include <iostream>
#include <utility>
#include <algorithm>
#include "core/Bar.h"
#include "core/Position.h"
#include "core/Trade.h"
#include "core/Account.h"
#include "strategy/IStrategy.h"
#include "engine/ThreadPool.h"

class ExecutionEngine {
public:
    explicit ExecutionEngine(std::shared_ptr<IStrategy> strategy)
        : strategy_{std::move(strategy)}, 
          account_{strategy_->getConfig().initialCapital},
          threadPool_{1} {}

    void run(const std::vector<Bar>& bars) {
        if (bars.empty()) return;

        strategy_->on_start(bars.front(), account_.getBalance());

        for (const auto& bar : bars) {
            checkSLTP(bar);

            auto fut = threadPool_.enqueue([this, &bar] {
                return strategy_->on_bar(bar, positions_, account_.getBalance());
            });
            auto action = fut.get();
            
            // Process close signals first
            if (action.closeCurrentPosition && !positions_.empty()) {
                closePosition(0, bar.close, bar.timestamp);
            }

            // Process open signals, only if no position is currently open
            if (positions_.empty()) {
                for (const auto& order : action.openRequests) {
                    processOpenOrder(order, bar);
                }
            }
        }
        strategy_->on_finish();
        account_.printSummary();
    }

private:
    void processOpenOrder(const OrderRequest& order, const Bar& currentBar) {
        Position newPosition;
        newPosition.side = order.side;
        newPosition.entryPrice = currentBar.close;
        newPosition.entryTimestamp = currentBar.timestamp;
        newPosition.sizeAmount = order.sizeUsd / currentBar.close;
        newPosition.leverage = order.leverage;

        if (order.stopLossPrice > 0) {
            newPosition.stopLossPrice = order.stopLossPrice;
        }
        if (order.takeProfitPrice > 0) {
            newPosition.takeProfitPrice = order.takeProfitPrice;
        }
        
        positions_.push_back(newPosition);
        std::cout << "EXEC: Opened " << (order.side == Side::Long ? "LONG" : "SHORT") 
                    << " position of " << newPosition.sizeAmount 
                    << " @ " << newPosition.entryPrice << std::endl;
    }

    void closePosition(std::size_t index, double exitPrice, std::int64_t exitTimestamp) {
        if (index >= positions_.size()) return;

        const auto& pos = positions_[index];
        Trade trade;
        trade.side = pos.side;
        trade.sizeAmount = pos.sizeAmount;
        trade.entryPrice = pos.entryPrice;
        trade.entryTimestamp = pos.entryTimestamp;
        trade.exitPrice = exitPrice;
        trade.exitTimestamp = exitTimestamp;
        
        double pnl = (trade.exitPrice - trade.entryPrice) * trade.sizeAmount;
        if (trade.side == Side::Short) {
            pnl = -pnl;
        }
        trade.pnl = pnl;

        account_.recordTrade(trade);
        positions_.erase(positions_.begin() + index);

        std::cout << "EXEC: Closed " << (trade.side == Side::Long ? "LONG" : "SHORT") 
                  << " position @ " << exitPrice << " for a PNL of " << pnl << std::endl;
    }

    void checkSLTP(const Bar& currentBar) {
        if (positions_.empty()) return;

        const auto& pos = positions_[0];
        if (pos.side == Side::Long) {
            if (pos.stopLossPrice > 0 && currentBar.low <= pos.stopLossPrice) {
                closePosition(0, pos.stopLossPrice, currentBar.timestamp);
            } else if (pos.takeProfitPrice > 0 && currentBar.high >= pos.takeProfitPrice) {
                closePosition(0, pos.takeProfitPrice, currentBar.timestamp);
            }
        } else { // Short position
            if (pos.stopLossPrice > 0 && currentBar.high >= pos.stopLossPrice) {
                closePosition(0, pos.stopLossPrice, currentBar.timestamp);
            } else if (pos.takeProfitPrice > 0 && currentBar.low <= pos.takeProfitPrice) {
                closePosition(0, pos.takeProfitPrice, currentBar.timestamp);
            }
        }
    }

    std::shared_ptr<IStrategy> strategy_;
    Account account_;
    ThreadPool threadPool_;

    std::vector<Position> positions_{};
}; 