#pragma once

#include <memory>
#include <vector>
#include "core/Bar.h"

class PriceSource {
public:
    virtual ~PriceSource() = default;
 // ... existing code ...
    virtual std::vector<Bar> fetch() = 0;  // blocking fetch of full dataset for now
};