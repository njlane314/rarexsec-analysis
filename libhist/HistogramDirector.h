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
            const auto histogram_model = this->createHistModel(binning);
            VariableFuture variable_future;
            variable_future.binning_ = binning;

            this->bookDataHists(binning, samples, histogram_model, variable_future);
            this->bookNominalHists(binning, samples, histogram_model, variable_future);
            this->bookVariationHists(binning, samples, histogram_model, variable_future);

            variable_map[variable_key] = std::move(variable_future);
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
                                VariableFuture& variable_future) = 0;

    virtual void bookNominalHists(const BinningDefinition& binning,
                                    const SampleEnsembleMap& samples,
                                    const ROOT::RDF::TH1DModel& model,
                                    VariableFuture& variable_future) = 0;

    virtual void bookVariationHists(const BinningDefinition& binning,
                                    const SampleEnsembleMap& samples,
                                    const ROOT::RDF::TH1DModel& model,
                                    VariableFuture& variable_future) = 0;
};

}

#endif