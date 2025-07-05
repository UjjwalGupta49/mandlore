#pragma once

#include "core/Trade.h"

struct Position {
    Side   side{Side::Long};
    double entryPrice{0.0};
    std::int64_t entryTimestamp{0};
    double sizeAmount{0.0};
    double leverage{1.0};
    double stopLossPrice{0.0};
    double takeProfitPrice{0.0};
}; 