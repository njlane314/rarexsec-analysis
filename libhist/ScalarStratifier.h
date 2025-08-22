#ifndef SCALAR_STRATIFIER_H
#define SCALAR_STRATIFIER_H

#include "IHistogramStratifier.h"
#include "StratifierRegistry.h"
#include "Keys.h"
#include <string>
#include <vector>

namespace analysis {

class ScalarStratifier : public IHistogramStratifier {
public:
    ScalarStratifier(const StratifierKey& key,
                     StratifierRegistry& registry)
      : stratifier_key_(key)
      , stratifier_registry_(registry)
    {}

protected:
    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                const BinningDefinition&,
                                int key) const override {
        return df.Filter(
            getSchemeName() + " == " + std::to_string(key)
        );
    }

    const std::string& getSchemeName() const override {
        return stratifier_key_.str();
    }

    const StratifierRegistry& getRegistry() const override {
        return stratifier_registry_;
    }

private:
    StratifierKey              stratifier_key_;
    StratifierRegistry&    stratifier_registry_;
};

}

#endif