#ifndef HISTOGRAM_BOOKER_H
#define HISTOGRAM_BOOKER_H

#include "AnalysisLogger.h"
#include "AnalysisTypes.h"
#include "IHistogramBooker.h"
#include "StratifierManager.h"
#include "StratifierRegistry.h"

namespace analysis {

class HistogramBooker : public IHistogramBooker {
  public:
    HistogramBooker(StratifierRegistry &strat_reg)
        : stratifier_manager_(strat_reg) {
        log::debug("HistogramBooker::HistogramBooker",
                   "Constructor called, StratifierManager has been created.");
    }

    ROOT::RDF::RResultPtr<TH1D>
    bookNominalHist(const BinningDefinition &binning,
                    const SampleDataset &dataset,
                    const ROOT::RDF::TH1DModel &model) override {
        auto d = dataset.dataframe_;
        return d.Histo1D(model, binning.getVariable(), "nominal_event_weight");
    }

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>>
    bookStratifiedHists(const BinningDefinition &binning,
                        const SampleDataset &dataset,
                        const ROOT::RDF::TH1DModel &model) override {
        analysis::log::info("HistogramBooker::bookStratifiedHists",
                            "Calling stratifier manager...");
        auto &stratifier = stratifier_manager_.get(binning.getStratifierKey());

        analysis::log::info("HistogramBooker::bookStratifiedHists",
                            "Creating stratified hists.");
        auto stratified_hists = stratifier.stratifyHist(
            dataset.dataframe_, binning, model, "nominal_event_weight");

        analysis::log::info(
            "HistogramBooker::bookStratifiedHists",
            "Variable created. About to return stratified hists.");
        return stratified_hists;
    }

  private:
    StratifierManager stratifier_manager_;
};

}

#endif
