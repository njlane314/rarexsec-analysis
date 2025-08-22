#ifndef VECTOR_STRATIFIER_H
#define VECTOR_STRATIFIER_H

#include "IHistogramStratifier.h"
#include "StratifierRegistry.h"
#include "ROOT/RVec.hxx"
#include "Keys.h"
#include <string>
#include <vector>
#include <functional>

namespace analysis {

class VectorStratifier : public IHistogramStratifier {
public:
    VectorStratifier(const StratifierKey& key,
                     StratifierRegistry& registry)
      : strat_key_(key)
      , strat_registry_(registry)
    {}

protected:
    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                const BinningDefinition&,
                                int key) const override {
        auto predicate = strat_registry_.findPredicate(strat_key_);
        return df.Filter(
            [predicate, key](const ROOT::RVec<int>& branch_values) {
                return predicate(branch_values, key);
            },
            {getSchemeName()}
        );
    }

    const std::string& getSchemeName() const override {
        return strat_key_.str();
    }

    const StratifierRegistry& getRegistry() const override {
        return strat_registry_;
    }

private:
    StratifierKey          strat_key_;
    StratifierRegistry&    strat_registry_;
};

}

#endif