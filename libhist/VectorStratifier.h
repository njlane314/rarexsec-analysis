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

    ROOT::RDF::RNode defineStratificationColumns(ROOT::RDF::RNode df,
                                                 const BinDefinition& bin) const override {
        auto modified_df = df;
        for (auto key : this->getRegistryKeys()) {
            auto selector = [key](const ROOT::RVec<float>& vals,
                        const ROOT::RVec<int>& ids) {
                            return vals[ROOT::VecOps::abs(ids) == key];
                        };
                        modified_df = modified_df.Define(getTempVariable(key), selector, {bin.getVariable().Data(), variable_});
        }
        return modified_df;
    }

protected:
    std::vector<int> getRegistryKeys() const override {
        return registry_.getStratumKeys(registry_key_);
    }

    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                const BinDefinition&,
                                int) const override {
        return df;
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