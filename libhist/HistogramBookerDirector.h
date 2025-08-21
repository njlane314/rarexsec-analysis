#ifndef HISTOGRAM_DIRECTOR_H
#define HISTOGRAM_DIRECTOR_H

#include "BinningDefinition.h"
#include "IHistogramBooker.h"
#include "RegionAnalysis.h"
#include "Keys.h"
#include "AnalysisTypes.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <vector>
#include <iterator>
#include <map>

namespace analysis {

class HistogramDirector : public IHistogramBooker {
public:
    std::unordered_map<VariableKey, VariableFuture> bookHistograms(
        const std::vector<std::pair<VariableKey, BinningDefinition>>& variable_definitions,
        const SampleEnsembleMap& samples
    ) override {
        std::unordered_map<VariableKey, VariableFuture> variable_map;

        for (const auto& [variable_key, binning] : variable_definitions) {
            const auto hist_model = this->createHistModel(binning);
            VariableFuture var_future;
            var_future.binning_ = binning;

            for (const auto& [sample_key, sample_ensemble] : sample_ensembles) {
                if (sample_ensembles.nominal.origin_ == SampleOrigin::kData) {
                    this->bookDataHists(binning, sample_ensemble.nominal_, hist_model, var_future);
                } else {
                    this->bookNominalHists(binning, sample_ensemble.nominal_, hist_model, var_future);

                    for (const auto& [variation_key, variation_dataset] : sample_ensemble.variations_) {
                        this->bookVariationHists(binning, variation_dataset, hist_model, variable_future);
                    }
                }
            } 

            variable_map[variable_key] = std::move(var_future);
        }

        return variable_map;
    }

protected:
    static ROOT::RDF::TH1DModel createHistModel(const BinningDefinition& binning) {
        return ROOT::RDF::TH1DModel(
            binning.getVariable().c_str(),
            binning.getTexLabel().c_str(),
            static_cast<int>(binning.getEdges().size() - 1),
            binning.getEdges().data()
        );
    }

    virtual void bookDataHists(const BinningDefinition& binning,
                               const SampleEnsembleMap& samples,
                               const ROOT::RDF::TH1DModel& model,
                               VariableFuture& var_future) = 0;

    virtual void bookNominalHists(const BinningDefinition& binning,
                                const SampleEnsembleMap& samples,
                                const ROOT::RDF::TH1DModel& model,
                                VariableFuture& var_future) = 0;

    virtual void bookVariationHists(const BinningDefinition& binning,
                                const SampleEnsembleMap& samples,
                                const ROOT::RDF::TH1DModel& model,
                                VariableFuture& var_future) = 0;
};

}

#endif