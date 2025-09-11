#ifndef VARIABLE_RESULT_H
#define VARIABLE_RESULT_H

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "TMatrixDSym.h"

#include <rarexsec/hist/BinnedHistogram.h>
#include <rarexsec/app/AnalysisKey.h>
#include <rarexsec/data/SampleTypes.h>

namespace analysis {

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

}

#endif
