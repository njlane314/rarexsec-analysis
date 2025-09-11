#ifndef VECTOR_STRATIFIER_H
#define VECTOR_STRATIFIER_H

#include <rarexsec/hist/IHistogramStratifier.h>
#include <rarexsec/core/AnalysisKey.h>
#include "ROOT/RVec.hxx"
#include <rarexsec/hist/StratifierRegistry.h>

#include <functional>
#include <memory>
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
        std::vector<std::string> columns = {this->getSchemeName()};

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

inline std::unique_ptr<IHistogramStratifier>
makeVectorStratifier(const StratifierKey &key, StratifierRegistry &registry) {
    return std::make_unique<VectorStratifier>(key, registry);
}

}

#endif
