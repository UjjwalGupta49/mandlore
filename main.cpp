#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "data/Aggregator.h"
#include "data/PriceManager.h"
#include "engine/ExecutionEngine.h"
#include "strategy/StrategyFactory.h"
#include "buy_and_hold/BuyAndHoldStrategy.h" // Include the strategy
#include "sma_cross/SmaCrossStrategy.h"

void printUsage(const StrategyFactory& factory) {
    std::cout << "Usage: ./backtest_runner <symbol> <resolution_minutes> <from_timestamp> <to_timestamp> <strategy_name>\n"
              << "Example: ./backtest_runner Crypto.BTC/USD 1 1684137600 1684141200 buy_and_hold\n\n"
              << "Available strategies:\n";
    for (const auto& name : factory.getRegisteredStrategies()) {
        std::cout << " - " << name << "\n";
    }
}

int main(int argc, char* argv[]) {
    // --- Strategy Registration ---
    StrategyFactory factory;
    factory.registerStrategy("buy_and_hold", [](const StrategyConfig& config) {
        return std::make_shared<BuyAndHoldStrategy>(config);
    });
    factory.registerStrategy("sma_cross", [](const StrategyConfig& config) {
        return std::make_shared<SmaCrossStrategy>(config);
    });
    // --- Register new strategies here ---


    if (argc != 6) {
        printUsage(factory);
        return 1;
    }

    try {
        std::string symbol = argv[1];
        int targetResolution = std::stoi(argv[2]); // The resolution we want to trade on
        long from = std::stol(argv[3]);
        long to = std::stol(argv[4]);
        std::string strategyName = argv[5];

        // 1. Get Data - Always fetch 1-minute data from the source to ensure we have
        // the finest granularity for aggregation.
        const std::string fetchResolution = "1";
        PriceManager priceManager(symbol, fetchResolution, from, to);
        auto rawBars = priceManager.loadData();
        if (rawBars.empty()) {
            std::cerr << "No data loaded for the given parameters. Exiting." << std::endl;
            return 1;
        }

        // 2. Aggregate Data to the user's desired trading resolution
        Aggregator aggregator(targetResolution);
        auto tradeBars = aggregator.aggregate(rawBars);
        std::cout << "Aggregated " << rawBars.size() << " raw bars into " 
                  << tradeBars.size() << " " << targetResolution << "-minute bars.\n";

        // 3. Set up Strategy and Engine
        auto strategy = factory.createStrategy(strategyName);
        ExecutionEngine engine(strategy);

        // 4. Run Backtest
        std::cout << "\n--- Running Backtest ---\n";
        engine.run(tradeBars);
        std::cout << "--- Backtest Finished ---\n";

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 