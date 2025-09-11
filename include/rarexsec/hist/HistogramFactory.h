#ifndef HISTOGRAM_FACTORY_H
#define HISTOGRAM_FACTORY_H

#include <rarexsec/utils/Logger.h>
#include <rarexsec/data/SampleDataset.h>
#include <rarexsec/hist/StratifierManager.h>
#include <rarexsec/hist/StratifierRegistry.h>

namespace analysis {

class HistogramFactory {
  public:
    HistogramFactory() : stratifier_manager_(stratifier_registry_) {
        log::debug("HistogramFactory::HistogramFactory", "Constructor called, StratifierManager has been created.");
    }

    ROOT::RDF::RResultPtr<TH1D> bookNominalHist(const BinningDefinition &binning, const SampleDataset &dataset,
                                                const ROOT::RDF::TH1DModel &model) {
        auto d = dataset.dataframe_;
        return d.Histo1D(model, binning.getVariable(), "nominal_event_weight");
    }

    std::unordered_map<StratumKey, ROOT::RDF::RResultPtr<TH1D>>
    bookStratifiedHists(const BinningDefinition &binning, const SampleDataset &dataset,
                        const ROOT::RDF::TH1DModel &model) {
        analysis::log::info("HistogramFactory::bookStratifiedHists", "Calling stratifier manager...");
        analysis::log::debug("HistogramFactory::bookStratifiedHists", "Binning requests stratifier key:",
                             binning.getStratifierKey().str());
        auto &stratifier = stratifier_manager_.get(binning.getStratifierKey());

        analysis::log::info("HistogramFactory::bookStratifiedHists", "Creating stratified hists.");
        auto stratified_hists = stratifier.stratifyHist(dataset.dataframe_, binning, model, "nominal_event_weight");

        analysis::log::info("HistogramFactory::bookStratifiedHists",
                            "Variable created. About to return stratified hists.");
        return stratified_hists;
    }

  private:
    StratifierRegistry stratifier_registry_;
    StratifierManager stratifier_manager_;
};

}

#endif
