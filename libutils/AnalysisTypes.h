#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include "ROOT/RDataFrame.hxx"
#include "SampleTypes.h"
#include "Keys.h"

namespace analysis {

class BinnedHistogram; 
class TMatrixDSym;

struct VariableResult {
    BinningDefinition binning_;
    BinnedHistogram data_hist_;
    BinnedHistogram total_mc_hist_;
    std::map<StratumKey, BinnedHistogram> strat_hists_;
    std::map<SystematicKey, TMatrixDSym> covariance_matrices_;
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