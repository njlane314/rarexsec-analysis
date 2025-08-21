#ifndef STRATIFIER_FACTORY_H
#define STRATIFIER_FACTORY_H

#include "IHistogramStratifier.h"
#include "StratificationRegistry.h"
#include "ScalarStratifier.h"
#include "VectorStratifier.h"
#include "Logger.h"
#include "Keys.h"
#include <memory>
#include <string>

namespace analysis {

class StratifierFactory {
public:
    static std::unique_ptr<IHistogramStratifier> create(
        const StratifierKey& key,
        StratificationRegistry& registry)
    {
        log::info("StratifierFactory", "Requested stratifier key:", key.str());

        StratifierType type = registry.getSchemeType(key);

        if (type == StratifierType::kScalar) {
            return std::make_unique<ScalarStratifier>(key, registry);
        }

        if (type == StratifierType::kVector) {
            return std::make_unique<VectorStratifier>(key, registry);
        }

        log::fatal("StratifierFactory", "Unknown or unregistered stratifier configuration:", key.str());
        return nullptr; 
    }
};

}

#endif