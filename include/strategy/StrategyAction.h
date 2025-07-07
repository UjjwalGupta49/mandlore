#pragma once

#include "core/OrderRequest.h"
#include <vector>

// Defines the set of actions a strategy can request on a given bar.
struct StrategyAction {
    // A list of new orders to be opened.
    std::vector<OrderRequest> openRequests;

    // If true, signals the engine to close the currently open position.
    bool closeCurrentPosition{false};
}; 