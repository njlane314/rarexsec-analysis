#ifndef HISTOGRAM_BOOKER_H
#define HISTOGRAM_BOOKER_H

#include "IHistogramBooker.h"
#include "StratifierFactory.h"
#include "StratifierRegistry.h"
#include "AnalysisTypes.h"

namespace analysis {

class HistogramBooker : public IHistogramBooker {
public:
    HistogramBooker(StratifierRegistry& strat_reg)
      : stratifier_registry_(strat_reg) {}

    ROOT::RDF::RResultPtr<TH1D> bookNominalHist(
        const BinningDefinition& binning,
        const AnalysisDataset& dataset,
        const ROOT::RDF::TH1DModel& model
    ) override {
        return dataset.dataframe_.Histo1D(model, binning.getVariable().c_str(), "nominal_event_weight");
    }

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>> bookStratifiedHists(
        const BinningDefinition& binning,
        const AnalysisDataset& dataset,
        const ROOT::RDF::TH1DModel& model
    ) override {
        const auto stratifier = StratifierFactory::create(binning.getStratifierKey(), stratifier_registry_);
        return stratifier->stratifyHist(dataset.dataframe_, binning, model, "nominal_event_weight");
    }

private:
    StratifierRegistry& stratifier_registry_;
};

}

#endif