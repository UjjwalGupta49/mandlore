#pragma once

#include <cstdint>

struct Bar {
    std::int64_t timestamp{0};   // epoch milliseconds
    double open{0.0};
    double high{0.0};
    double low{0.0};
    double close{0.0};
    double volume{0.0};
    std::int64_t num_trades{0};  // number of trades
}; 