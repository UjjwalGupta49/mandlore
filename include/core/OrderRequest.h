#pragma once

#include <cstdint>

enum class Side { Long, Short };

struct OrderRequest {
    Side   side{Side::Long};
    double sizeUsd{0.0};     // position notional in USD
    double sizeAmount{0.0};  // asset quantity (e.g., BTC)
    double leverage{1.0};
    double stopLossPrice{0.0};
    double takeProfitPrice{0.0};
}; 