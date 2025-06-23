#pragma once

#include <cstddef>
#include <vector>
#include "core/Bar.h"

// Simple N-minute bar aggregator (placeholder)
class Aggregator {
public:
    explicit Aggregator(std::size_t resolutionMinutes) : resolution_{resolutionMinutes} {}

    std::vector<Bar> aggregate(const std::vector<Bar>& raw) const;

private:
    std::size_t resolution_;
}; 