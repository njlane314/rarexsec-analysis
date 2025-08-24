#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <unordered_map>
#include "ROOT/RDataFrame.hxx"
#include "SampleTypes.h"
#include "Keys.h"
#include "BinnedHistogram.h"
#include "TMatrixDSym.h"

namespace analysis {

class RegionAnalysis;

struct VariableResult {
    BinningDefinition binning_;
    BinnedHistogram   data_hist_;
    BinnedHistogram   total_mc_hist_;
    std::map<ChannelKey, BinnedHistogram> strat_hists_;

    std::map<SampleKey, std::map<SampleVariation, BinnedHistogram>> raw_detvar_hists_;

    std::map<SystematicKey, BinnedHistogram> variation_hists_;
    std::map<SystematicKey, BinnedHistogram> transfer_ratio_hists_;
    std::map<SystematicKey, BinnedHistogram> delta_hists_;
    std::map<SystematicKey, TMatrixDSym> covariance_matrices_;

    TMatrixDSym total_covariance_;
    TMatrixDSym total_correlation_;
    BinnedHistogram nominal_with_band_;

    std::map<SystematicKey, std::vector<BinnedHistogram>> universe_projected_hists_;
};

using AnalysisRegionMap = std::map<RegionKey, RegionAnalysis>;

struct AnalysisDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    mutable ROOT::RDF::RNode dataframe_;
};

struct SampleEnsemble {
    AnalysisDataset nominal_;
    std::map<SampleVariation, AnalysisDataset> variations_;
};

using SampleEnsembleMap = std::map<SampleKey, SampleEnsemble>;

}

#endif