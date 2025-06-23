 

#include <vector>
#include "core/Bar.h"
#include "core/Position.h"
#include "core/OrderRequest.h"

class IStrategy {
public:
    virtual ~IStrategy() = default;

    virtual void on_start(const Bar& firstBar, double initialEquity) = 0;

    virtual std::vector<OrderRequest> on_bar(const Bar& currentBar,
                                             const std::vector<Position>& openPositions,
                                             double accountEquity) = 0;

    virtual void on_finish() = 0;
};