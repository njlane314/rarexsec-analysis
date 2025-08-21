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
#include "HistogramBuilderFactory.h"
#include "StratifierRegistry.h"
#include "Logger.h"
#include "Keys.h"

namespace analysis {

class IHistogramStratifier {
public:
    virtual ~IHistogramStratifier() = default;

    virtual std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> bookHistogram(ROOT::RDF::RNode dataframe,
                                            const BinDefinition& binning,
                                            const ROOT::RDF::TH1DModel& hist_model,
                                            const std::string& weight_column) const
    {
        std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> strat_futurs;
        for (const auto& key : this->getRegistryKeys()) {
            auto slice = this->filterNode(dataframe, binning, std::stoi(key.str()));
            strat_futurs[key] = slice.Histo1D(
                hist_model,
                binning.getVariable().Data(),
                weight_column.c_str()
            );
        }
        return strat_futurs;
    }

protected:
    virtual ROOT::RDF::RNode filterNode(ROOT::RDF::RNode dataframe,
                                        const BinDefinition& binning,
                                        int key) const = 0;

    virtual const std::string& getSchemeName() const = 0;
    virtual const StratifierRegistry& getRegistry() const = 0;

    std::vector<StratumKey> getRegistryKeys() const {
        return this->getRegistry().getAllStratumKeysForScheme(this->getSchemeName());
    }
};

}

#endif