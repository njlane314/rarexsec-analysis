#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "TMatrixDSym.h"

#include "BinnedHistogram.h"
#include "KeyTypes.h"
#include "SampleTypes.h"

namespace analysis {

class RegionAnalysis;

struct VariableResult {
    BinningDefinition binning_;
    BinnedHistogram data_hist_;
    BinnedHistogram total_mc_hist_;
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

using RegionAnalysisMap = std::map<RegionKey, RegionAnalysis>;

struct SampleDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    mutable ROOT::RDF::RNode dataframe_;
};

struct SampleDatasetGroup {
    SampleDataset nominal_;
    std::unordered_map<SampleVariation, SampleDataset> variations_;
};

using SampleDatasetGroupMap = std::unordered_map<SampleKey, SampleDatasetGroup>;

} // namespace analysis

#endif
