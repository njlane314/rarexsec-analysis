#include "IHistogramStratifier.h"
#include "KeyTypes.h"
#include "ROOT/RVec.hxx"
#include "StratifierManager.h"
#include "StratifierRegistry.h"
#include "ScalarStratifier.h"
#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace analysis {

class VectorStratifier : public IHistogramStratifier {
  public:
    VectorStratifier(const StratifierKey &key, StratifierRegistry &registry)
        : strat_key_(key), strat_registry_(registry) {}

  protected:
    ROOT::RDF::RNode defineFilterColumn(ROOT::RDF::RNode dataframe, int key,
                                        const std::string &new_column_name) const override {

        std::vector<std::string> columns = {getSchemeName()};

        return dataframe.Define(
            new_column_name,
            [this, key](const ROOT::RVec<int> &branch_values) {
                auto predicate = this->strat_registry_.findPredicate(this->strat_key_);
                return predicate(branch_values, key);
            },
            columns);
    }

    const std::string &getSchemeName() const override { return strat_key_.str(); }

    const StratifierRegistry &getRegistry() const override { return strat_registry_; }

  private:
    StratifierKey strat_key_;
    StratifierRegistry &strat_registry_;
};

IHistogramStratifier &StratifierManager::get(const StratifierKey &key) {
    log::debug("StratifierManager::get", "Attempting to get stratifier for key:", key.str());

    auto it = cache_.find(key);

    if (it == cache_.end()) {
        log::info("StratifierManager::get", "Creating new stratifier for key:", key.str());

        std::unique_ptr<IHistogramStratifier> stratifier;

        StratifierType type = registry_.findSchemeType(key);

        if (type == StratifierType::kScalar) {
            stratifier = std::make_unique<ScalarStratifier>(key, registry_);
        } else if (type == StratifierType::kVector) {
            stratifier = std::make_unique<VectorStratifier>(key, registry_);
        } else {
            log::fatal("StratifierManager::get", "Unknown or unregistered stratifier configuration:", key.str());
        }

        it = cache_.emplace(key, std::move(stratifier)).first;

        log::debug("StratifierManager::get", "Successfully created and cached stratifier for key:", key.str());
    } else {
        log::debug("StratifierManager::get", "Found cached stratifier for key:", key.str());
    }

    return *it->second;
}

}
