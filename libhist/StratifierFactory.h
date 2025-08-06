#ifndef STRATIFIER_FACTORY_H
#define STRATIFIER_FACTORY_H

#include "IHistogramStratifier.h"
#include "StratificationRegistry.h"
#include "ScalarStratifier.h"
#include "VectorStratifier.h"
#include "Logger.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>

namespace analysis {

class StratifierFactory {
    using StratifierCreator = std::function<std::unique_ptr<IHistogramStratifier>(const std::string& cfg, StratificationRegistry&)>;
    static const std::unordered_map<std::string, StratifierCreator> creators_map_;

public:
    static std::unique_ptr<IHistogramStratifier> create(
        const std::string& keycfg,
        StratificationRegistry& registry)
    {
        auto [key, cfg] = splitOnce(keycfg, ':');
        auto it = creators_map_.find(key);
        if (it == creators_map_.end())
            log::fatal("StratifierFactory", "Unknown strategy key: " + key);
        return it->second(cfg, registry);
    }
};

const std::unordered_map<std::string, StratifierFactory::StratifierCreator>
StratifierFactory::creators_map_ = {
    { "scalar", [](auto& cfg, auto& reg){
        const auto& keys = reg.getStratumKeys(cfg);
        if (keys.empty())
            log::fatal("StratifierFactory", "No strata for " + cfg);
        return std::make_unique<ScalarStratifier>(cfg, cfg, reg);
    }},
    { "vector", [](auto& cfg, auto& reg){
        const auto& keys = reg.getStratumKeys(cfg);
        if (keys.empty())
            log::fatal("StratifierFactory", "No strata for " + cfg);
        return std::make_unique<VectorStratifier>(cfg, cfg, reg);
    }}
};

} 

#endif // STRATIFIER_FACTORY_H
