#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <tuple>
#include "ROOT/RDataFrame.hxx"
#include "SampleTypes.h"
#include "Keys.h"

namespace analysis {

using TH1DFuture = ROOT::RDF::RResultPtr<TH1D>;

struct VariableFuture {
    BinDefinition binning_;

    TH1DFuture data_hist_;
    TH1DFuture total_hist_;
    std::unordered_map<StratumKey, TH1DFuture> strat_hists_;

    std::unordered_map<VariationKey, TH1DFuture> variation_hists_;
};

struct VariableResult {
    BinDefinition binning_;

    BinnedHistogram data_hist_;
    BinnedHistogram total_hist_;
    std::map<StratumKey, BinnedHistogram> strat_hists_;

    std::map<std::string, std::map<std::string, std::map<int, BinnedHistogram>>> variation_hists_;
    std::map<int, std::map<std::string, TMatrixDSym>> covariance_matrices;
};

using AnalysisRegionMap = std::unordered_map<RegionKey, RegionAnalysis>;

struct AnalysisDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    ROOT::RDF::RNode dataframe_;
}

struct SampleEnsemble {
    AnalysisDataset nominal_;
    std::map<SampleVariation, AnalysisDataset> variations_;
}

using SampleEnsembleMap = std::map<SampleKey, SampleEnsemble>;

}

#endif