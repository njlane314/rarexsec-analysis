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
      : stratifier_key_(key)
      , registry_(registry)
    {}

protected:
    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                const BinDefinition&,
                                int key) const override {
        auto predicate = registry_.findPredicate(stratifier_key_);
        return df.Filter(
            [predicate, key](const ROOT::RVec<int>& branch_values) {
                return predicate(branch_values, key);
            },
            {getSchemeName()}
        );
    }

    const std::string& getSchemeName() const override {
        return stratifier_key_.str();
    }

    const StratifierRegistry& getRegistry() const override {
        return registry_;
    }

private:
    StratifierKey          stratifier_key_;
    StratifierRegistry&    registry_;
};

}

#endif