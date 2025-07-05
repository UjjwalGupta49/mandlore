#pragma once

#include <cstdint>
#include "core/OrderRequest.h"

struct Trade {
    Side side{Side::Long};
    
    std::int64_t entryTimestamp{0};
    double entryPrice{0.0};

    std::int64_t exitTimestamp{0};
    double exitPrice{0.0};
    
    double sizeAmount{0.0};
    double pnl{0.0};
    double fee{0.0};
}; 