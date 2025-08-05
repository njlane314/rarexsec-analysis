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

    BranchType getRequiredBranchType() const override {
        return BranchType::Vector;
    }

protected:
    std::vector<int> getRegistryKeys() const override {
        return registry_.getStratumKeys(registry_key_);
    }

    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                  int key) const override {
        std::string col = variable_ + "_" + std::to_string(key);
        auto selector = [key](const ROOT::RVec<float>& vals,
                              const ROOT::RVec<int>& ids) {
            return vals[ROOT::VecOps::Abs(ids) == key];
        };
        return df.Define(col, selector, { variable_, variable_ });
    }

    std::string getTempVariable(int key) const override {
        return variable_ + "_" + std::to_string(key);
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