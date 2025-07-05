#pragma once

#include "core/Trade.h"
#include <vector>
#include <cstdint>

class Account {
public:
    explicit Account(double initialBalance);

    void recordTrade(const Trade& trade);

    void printSummary() const;

    [[nodiscard]] double getBalance() const { return balance_; }

    void setBalance(double balance) { balance_ = balance; }

private:
    double initialBalance_;
    double balance_;
    std::vector<Trade> closedTrades_;
}; 