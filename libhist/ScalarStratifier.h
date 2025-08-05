#ifndef SCALAR_STRATIFIER_H
#define SCALAR_STRATIFIER_H

#include "IHistogramStratifier.h"
#include "StratificationRegistry.h"
#include <string>
#include <vector>

namespace analysis {

class ScalarStratifier : public IHistogramStratifier {
public:
    ScalarStratifier(std::string key,
                     std::string variable,
                     StratificationRegistry& registry)
      : registry_key_(std::move(key))
      , variable_(std::move(variable))
      , registry_(registry)
    {}

    BranchType getRequiredBranchType() const override {
        return BranchType::Scalar;
    }

protected:
    std::vector<int> getRegistryKeys() const override {
        return registry_.getStratumKeys(registry_key_);
    }

    ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                int key) const override {
        return df.Filter(
            variable_ + " == " + std::to_string(key)
        );
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

#endif // SCALAR_STRATIFIER_H