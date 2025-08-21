#ifndef IHISTOGRAM_STRATIFIER_H
#define IHISTOGRAM_STRATIFIER_H

#include <map>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "ROOT/RDataFrame.hxx"
#include "TH1D.h"
#include "BinDefinition.h"
#include "BinnedHistogram.h"
#include "HistogramResult.h"
#include "HistogramBuilderFactory.h"
#include "StratificationRegistry.h"
#include "Logger.h"

namespace analysis {

class IHistogramStratifier {
public:
    using FutureMap = std::unordered_map<int, ROOT::RDF::RResultPtr<TH1D>>;

    virtual ~IHistogramStratifier() = default;

    virtual FutureMap bookHistogram(ROOT::RDF::RNode df,
                                            const BinDefinition& bin,
                                            const ROOT::RDF::TH1DModel& model) const
    {
        FutureMap out;
        for (auto key : this->getRegistryKeys()) {
            auto slice = this->filterNode(df, bin, key);
            out[key] = slice.Histo1D(
                model,
                bin.getVariable().Data(),
                "central_value_weight"
            );
        }
        return out;
    }

protected:
    virtual ROOT::RDF::RNode filterNode(ROOT::RDF::RNode df,
                                        const BinDefinition& bin,
                                        int key) const = 0;

    virtual const std::string& getRegistryKey() const = 0;
    virtual const std::string& getVariable() const = 0;
    virtual const StratificationRegistry& getRegistry() const = 0;

    std::vector<int> getRegistryKeys() const {
        return this->getRegistry().getStratumKeys(this->getRegistryKey());
    }
};

}

#endif