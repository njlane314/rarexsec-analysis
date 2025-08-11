#ifndef VECTOR_STRATIFIER_H
#define VECTOR_STRATIFIER_H

#include "IHistogramStratifier.h"
#include "StratificationRegistry.h"
#include "ROOT/RVec.hxx"
#include <string>
#include <vector>

namespace analysis {

class VectorStratifier : public IHistogramStratifier {
public:
    VectorStratifier(std::string key,
                     std::string variable,
                     StratificationRegistry& registry)
      : registry_key_(std::move(key))
      , variable_(std::move(variable))
      , registry_(registry)
    {}

protected:
    std::vector<int> getRegistryKeys() const override {
        return registry_.getStratumKeys(registry_key_);
    }

    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                const BinDefinition&,
                                int key) const override {
            [key](const ROOT::RVec<int>& ids) {
                return ROOT::VecOps::Sum(ROOT::VecOps::abs(ids) == key) > 0;
            },
            {variable_}
        );
    }

    std::string getTempVariable(int) const override {
        return variable_; 
    }

    const std::string& getRegistryKey() const override {
        return registry_key_;
    }

    const std::string& getVariable() const override {
        return variable_;
    }

    const StratificationRegistry& getRegistry() const override {
        return registry_;
    }

private:
    std::string                registry_key_;
    std::string                variable_;
    StratificationRegistry&    registry_;
};

}

#endif // VECTOR_STRATIFIER_H