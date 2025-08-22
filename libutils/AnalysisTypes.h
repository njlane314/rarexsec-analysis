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

namespace analysis {

struct VariableResult {
    BinningDefinition binning_;
    BinnedHistogram   data_hist_;
    BinnedHistogram   total_mc_hist_;  
    std::map<StratumKey, BinnedHistogram> strat_hists_; // stack components, if you show them

    // === Variations projected onto the nominal (for ratio panels, envelopes, etc.)
    // Semantics: these ARE h_proj^(k) = R^(k) ⊙ total_mc_hist_
    std::map<SystematicKey, BinnedHistogram> variation_hists_;

    // === Bin-by-bin transfer shapes inside the DetVar family
    // R^(k) = (detvar_k / detvar_cv) after per-POT (or area) normalization
    std::map<SystematicKey, BinnedHistogram> transfer_ratio_hists_;

    // === Deltas relative to nominal: Δ^(k) = variation_hists_[k] - total_mc_hist_
    std::map<SystematicKey, BinnedHistogram> delta_hists_;

    // === Covariances (split per systematic) and totals
    std::map<SystematicKey, TMatrixDSym> covariance_matrices_; // detvar, flux, xsec, stat, ...
    TMatrixDSym total_covariance_;
    TMatrixDSym total_correlation_;  // derived from total_covariance_

    // === Convenience: nominal with 1σ band (bin errors = sqrt(diag(total_covariance_)))
    BinnedHistogram nominal_with_band_;

    // === Optional: for multisim QA (universes after projection)
    std::map<SystematicKey, std::vector<BinnedHistogram>> universe_projected_hists_;
};

using AnalysisRegionMap = std::unordered_map<RegionKey, RegionAnalysis>;

struct AnalysisDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    ROOT::RDF::RNode dataframe_;
};

struct SampleEnsemble {
    AnalysisDataset nominal_;
    std::map<SampleVariation, AnalysisDataset> variations_;
};

using SampleEnsembleMap = std::map<SampleKey, SampleEnsemble>;

}

#endif
