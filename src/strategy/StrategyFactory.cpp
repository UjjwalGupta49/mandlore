#include "strategy/StrategyFactory.h"
#include <stdexcept>

void StrategyFactory::registerStrategy(const std::string& name, TCreateMethod createMethod) {
    registry_[name] = std::move(createMethod);
}

std::shared_ptr<IStrategy> StrategyFactory::createStrategy(const std::string& name) {
    auto it = registry_.find(name);
    if (it == registry_.end()) {
        throw std::runtime_error("Unknown strategy: " + name);
    }
    return it->second();
}

std::vector<std::string> StrategyFactory::getRegisteredStrategies() const {
    std::vector<std::string> names;
    for (const auto& pair : registry_) {
        names.push_back(pair.first);
    }
    return names;
} 