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

inline std::pair<std::string, std::string> splitOnce(const std::string& s, char delimiter) {
    auto pos = s.find(delimiter);
    if (pos == std::string::npos) {
        return {s, ""};
    }
    return {s.substr(0, pos), s.substr(pos + 1)};
}

class StratifierFactory {
    using StratifierCreator = std::function<std::unique_ptr<IHistogramStratifier>(const std::string& cfg, StratificationRegistry&)>;
    static const std::unordered_map<std::string, StratifierCreator> creators_map_;

public:
    static std::unique_ptr<IHistogramStratifier> create(
        const std::string& keycfg,
        StratificationRegistry& registry)
    {
        log::info("StratifierFactory", "Requested stratifier keycfg:", keycfg);
        auto [key, cfg] = splitOnce(keycfg, ':');
        auto it = creators_map_.find(key);
        if (it == creators_map_.end())
            log::fatal("StratifierFactory", "Unknown strategy key: " + key);
        return it->second(cfg, registry);
    }
};

const std::unordered_map<std::string, StratifierFactory::StratifierCreator>
StratifierFactory::creators_map_ = {
    { "scalar", [](const auto& configuration, auto& registry){
        const auto& stratum_keys = registry.getAllStratumKeys(configuration);
        if (stratum_keys.empty()) {
            log::fatal("StratifierFactory", "No strata found for scheme: " + configuration);
        }
        return std::make_unique<ScalarStratifier>(configuration, configuration, registry);
    }},
    { "vector", [](const auto& configuration, auto& registry){
        const auto& stratum_keys = registry.getAllStratumKeys(configuration);
        if (stratum_keys.empty()) {
            log::fatal("StratifierFactory", "No strata found for scheme: " + configuration);
        }
        return std::make_unique<VectorStratifier>(configuration, configuration, registry);
    }}
};

} 

#endif // STRATIFIER_FACTORY_H
