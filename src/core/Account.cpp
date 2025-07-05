#include "core/Account.h"
#include <iostream>
#include <iomanip>
#include <numeric>
#include <cmath>
#include <algorithm>

Account::Account(double initialBalance) : initialBalance_(initialBalance), balance_(initialBalance) {}

void Account::recordTrade(const Trade& trade) {
    closedTrades_.push_back(trade);
    balance_ += trade.pnl; // Update balance with the result of the trade
}

void Account::printSummary() const {
    if (closedTrades_.empty()) {
        std::cout << "\n--- No Trades Executed ---\n";
        std::cout << "Starting Balance: " << std::fixed << std::setprecision(2) << initialBalance_ << std::endl;
        std::cout << "Ending Balance:   " << std::fixed << std::setprecision(2) << balance_ << std::endl;
        return;
    }

    int totalTrades = closedTrades_.size();
    int winCount = 0;
    double grossPnl = 0.0;
    
    int longTrades = 0;
    double netPnlLong = 0.0;
    int shortTrades = 0;
    double netPnlShort = 0.0;

    std::vector<double> pnlValues;
    pnlValues.reserve(totalTrades);

    for (const auto& trade : closedTrades_) {
        pnlValues.push_back(trade.pnl);
        grossPnl += std::abs(trade.pnl);
        if (trade.pnl > 0) {
            winCount++;
        }
        if (trade.side == Side::Long) {
            longTrades++;
            netPnlLong += trade.pnl;
        } else {
            shortTrades++;
            netPnlShort += trade.pnl;
        }
    }

    double netPnl = balance_ - initialBalance_;
    double winLossRatio = (totalTrades > 0) ? static_cast<double>(winCount) / totalTrades : 0.0;

    // Sharpe Ratio Calculation
    double pnlMean = netPnl / totalTrades;
    double pnlStdDev = 0.0;
    if (totalTrades > 1) {
        double sq_sum = std::inner_product(pnlValues.begin(), pnlValues.end(), pnlValues.begin(), 0.0);
        pnlStdDev = std::sqrt(sq_sum / totalTrades - pnlMean * pnlMean);
    }
    double sharpeRatio = (pnlStdDev > 0) ? pnlMean / pnlStdDev : 0.0;


    // --- Print Summary ---
    std::cout << "\n--- Backtest Summary ---\n";
    std::cout << std::left << std::setw(20) << "Metric" << "Value\n";
    std::cout << "-----------------------------------\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << std::left << std::setw(20) << "Starting Balance:" << initialBalance_ << "\n";
    std::cout << std::left << std::setw(20) << "Ending Balance:" << balance_ << "\n";
    std::cout << std::left << std::setw(20) << "Net PNL:" << netPnl << "\n";
    std::cout << std::left << std::setw(20) << "Gross PNL:" << grossPnl << "\n";
    std::cout << std::left << std::setw(20) << "Total Trades:" << totalTrades << "\n";
    std::cout << std::left << std::setw(20) << "Win/Loss Ratio:" << winLossRatio * 100 << "%\n";
    std::cout << std::left << std::setw(20) << "Sharpe Ratio:" << sharpeRatio << "\n";
    std::cout << "-----------------------------------\n";
    std::cout << std::left << std::setw(20) << "Total Long Trades:" << longTrades << "\n";
    std::cout << std::left << std::setw(20) << "Net PNL Long:" << netPnlLong << "\n";
    std::cout << std::left << std::setw(20) << "Total Short Trades:" << shortTrades << "\n";
    std::cout << std::left << std::setw(20) << "Net PNL Short:" << netPnlShort << "\n";
    std::cout << "-----------------------------------\n";
} 