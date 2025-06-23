#pragma once

#include "core/Trade.h"

struct Position {
    Side   side{Side::Long};
    double entryPrice{0.0};
    double sizeAmount{0.0};
    double leverage{1.0};

    // unrealized PnL helper (placeholder)
    [[nodiscard]] double unrealized(double lastPrice) const noexcept {
        if (sizeAmount == 0.0) return 0.0;
        double pnl = (lastPrice - entryPrice) * sizeAmount;
        return (side == Side::Long) ? pnl : -pnl;
    }
}; 