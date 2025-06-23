#pragma once

#include <cstdint>
#include "core/OrderRequest.h"

struct Trade {
    std::int64_t timestamp{0};
    Side   side{Side::Long};
    double price{0.0};
    double sizeAmount{0.0};
    double fee{0.0};
}; 