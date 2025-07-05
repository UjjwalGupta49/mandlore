#include "strategy/StrategyFactory.h"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <stdexcept>

void from_json(const nlohmann::json& j, StrategyConfig& c) {
    j.at("initialCapital").get_to(c.initialCapital);
    j.at("stopLossPercent").get_to(c.stopLossPercent);
    j.at("takeProfitPercent").get_to(c.takeProfitPercent);
    j.at("perTradeSize").get_to(c.perTradeSize);
}

void StrategyFactory::registerStrategy(const std::string& name, TCreateMethod createMethod) {
    registry_[name] = std::move(createMethod);
}

std::shared_ptr<IStrategy> StrategyFactory::createStrategy(const std::string& name) {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        throw std::runtime_error("Strategy not found: " + name);
    }

    // Load and parse config file
    const std::string configPath = "strategies/" + name + "/config.json";
    std::ifstream f(configPath);
    if (!f.is_open()) {
        throw std::runtime_error("Could not open config file for strategy: " + name);
    }

    nlohmann::json data = nlohmann::json::parse(f);
    StrategyConfig config = data.get<StrategyConfig>();

    return it->second(config);
}

std::vector<std::string> StrategyFactory::getRegisteredStrategies() const {
    std::vector<std::string> names;
    names.reserve(registry_.size());
    for (const auto& pair : registry_) {
        names.push_back(pair.first);
    }
    return names;
} 