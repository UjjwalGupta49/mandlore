#pragma once

#include "strategy/IStrategy.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

// A factory for creating strategy instances from a string name.
class StrategyFactory {
public:
    using TCreateMethod = std::function<std::shared_ptr<IStrategy>()>;

    // Registers a new strategy creation method.
    void registerStrategy(const std::string& name, TCreateMethod createMethod);

    // Creates a strategy instance by name.
    std::shared_ptr<IStrategy> createStrategy(const std::string& name);

    // Returns a list of all registered strategy names.
    std::vector<std::string> getRegisteredStrategies() const;

private:
    std::map<std::string, TCreateMethod> registry_;
}; 