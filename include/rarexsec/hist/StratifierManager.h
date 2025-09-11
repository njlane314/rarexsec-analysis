#ifndef STRATIFIER_MANAGER_H
#define STRATIFIER_MANAGER_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <rarexsec/utils/Logger.h>
#include <rarexsec/hist/IHistogramStratifier.h>
#include <rarexsec/core/AnalysisKey.h>
#include <rarexsec/hist/StratifierRegistry.h>
#include <rarexsec/hist/ScalarStratifier.h>
#include <rarexsec/hist/VectorStratifier.h>

namespace analysis {

class StratifierManager {
  public:
    explicit StratifierManager(StratifierRegistry &registry) : registry_(registry) {}

    IHistogramStratifier &get(const StratifierKey &key) {
        log::debug("StratifierManager::get", "Attempting to get stratifier for key:", key.str());

        auto it = cache_.find(key);

        if (it == cache_.end()) {
            log::info("StratifierManager::get", "Creating new stratifier for key:", key.str());

            std::unique_ptr<IHistogramStratifier> stratifier;

            StratifierType type = registry_.findSchemeType(key);

            if (type == StratifierType::kScalar) {
                stratifier = makeScalarStratifier(key, registry_);
            } else if (type == StratifierType::kVector) {
                stratifier = makeVectorStratifier(key, registry_);
            } else {
                auto available = registry_.getRegisteredSchemeNames();
                std::string joined;
                for (const auto &name : available) {
                    joined += name + ",";
                }
                if (!joined.empty()) {
                    joined.pop_back();
                }
                log::warn("StratifierManager::get", "Available stratifier schemes:", joined);
                log::fatal("StratifierManager::get", "Unknown or unregistered stratifier configuration:", key.str());
            }

            it = cache_.emplace(key, std::move(stratifier)).first;

            log::debug("StratifierManager::get", "Successfully created and cached stratifier for key:", key.str());
        } else {
            log::debug("StratifierManager::get", "Found cached stratifier for key:", key.str());
        }

        return *it->second;
    }

  private:
    StratifierRegistry &registry_;
    std::unordered_map<StratifierKey, std::unique_ptr<IHistogramStratifier>> cache_;
};

}

#endif
