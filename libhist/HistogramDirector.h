#ifndef HISTOGRAM_DIRECTOR_H
#define HISTOGRAM_DIRECTOR_H

#include "BinDefinition.h"
#include "IHistogramBuilder.h"
#include "RegionAnalysis.h"
#include "Keys.h"
#include "ROOT/RDataFrame.hxx"
#include <TH1D.h>
#include <vector>
#include <iterator>

namespace analysis {

class HistogramDirector : public IHistogramBuilder {
public:
    RegionAnalysis buildRegionAnalysis(const RegionKey& region,
                                        const std::vector<std::pair<VariableKey, BinDefinition>>& variable_definitions,
                                        const SampleDataFrameMap& samples) override {
        RegionAnalysis region_analysis(region);
        
        for (const auto& [variable, binning] : variable_definitions) {
            const auto histogram_model = createHistogramModel(binning);
            VariableHistogramFutures variable_futures;
            variable_futures.binning = binning;
            variable_futures.axis_label = binning.getName().Data();
            
            buildDataHistograms(binning, samples, histogram_model, variable_futures);
            buildNominalHistograms(binning, samples, histogram_model, variable_futures);
            buildSystematicVariations(binning, samples, histogram_model, variable_futures);
            
            region_analysis.addVariableHistograms(variable, std::move(variable_futures));
        }
        
        return region_analysis;
    }
    


protected:
    static ROOT::RDF::TH1DModel createHistogramModel(const BinDefinition& binning) {
        return ROOT::RDF::TH1DModel(
            binning.getName().Data(),
            binning.getName().Data(),
            static_cast<int>(binning.nBins()),
            binning.getLowEdge(),
            binning.getHighEdge()
        );
    }

    virtual void buildDataHistograms(const BinDefinition& binning,
                                     const SampleDataFrameMap& samples,
                                     const ROOT::RDF::TH1DModel& model,
                                     VariableHistogramFutures& variable_futures) = 0;

    virtual void buildNominalHistograms(const BinDefinition& binning,
                                        const SampleDataFrameMap& samples,
                                        const ROOT::RDF::TH1DModel& model,
                                        VariableHistogramFutures& variable_futures) = 0;

    virtual void buildSystematicVariations(const BinDefinition& binning,
                                           const SampleDataFrameMap& samples,
                                           const ROOT::RDF::TH1DModel& model,
                                           VariableHistogramFutures& variable_futures) = 0;
};

}

#endif