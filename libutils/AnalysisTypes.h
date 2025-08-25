#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <iomanip>
#include <iostream>
#include <algorithm>
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

    std::map<SampleKey, std::map<SampleVariation, BinnedHistogram>>
        raw_detvar_hists_;

    std::map<SystematicKey, BinnedHistogram> variation_hists_;
    std::map<SystematicKey, BinnedHistogram> transfer_ratio_hists_;
    std::map<SystematicKey, BinnedHistogram> delta_hists_;
    std::map<SystematicKey, TMatrixDSym> covariance_matrices_;

    TMatrixDSym total_covariance_;
    TMatrixDSym total_correlation_;
    BinnedHistogram nominal_with_band_;

    std::map<SystematicKey, std::vector<BinnedHistogram>>
        universe_projected_hists_;

    void printSummary() const {
        std::cout
            << "\n+------------------------------------------------------+"
            << std::endl;
        std::cout << "| Analysis Result Summary for Variable: " << std::left
                  << std::setw(20) << binning_.getVariable() << " |"
                  << std::endl;
        std::cout << "+------------------------------------------------------+"
                  << std::endl;
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "| Total MC Events: " << std::left << std::setw(34)
                  << total_mc_hist_.getSum() << " |" << std::endl;
        std::cout << "+------------------------------------------------------+"
                  << std::endl;
        std::cout << "| Stratum Breakdown:" << std::string(35, ' ') << "|"
                  << std::endl;
        for (const auto &[key, hist] : strat_hists_) {
            std::cout << "|   - " << std::left << std::setw(20) << key.str()
                      << ": " << std::left << std::setw(24) << hist.getSum()
                      << " |" << std::endl;
        }
        std::cout << "+------------------------------------------------------+"
                  << std::endl;
        std::cout << "| Calculated Systematics:" << std::string(29, ' ') << "|"
                  << std::endl;
        for (const auto &[key, cov] : covariance_matrices_) {
            if (cov.GetNrows() > 0) {
                std::cout << "|   - " << std::left << std::setw(46) << key.str()
                          << " |" << std::endl;
                for (int i = 0; i < cov.GetNrows(); ++i) {
                    std::cout << "|       " << std::setw(8) << "";
                    for (int j = 0; j < cov.GetNcols(); ++j) {
                        std::cout << std::setw(10) << cov(i, j);
                    }
                    std::cout << std::string(
                                    std::max(0, 46 - 10 * cov.GetNcols()), ' ')
                              << "|" << std::endl;
                }
            }
        }
        std::cout
            << "+------------------------------------------------------+\n"
            << std::endl;
    }
};

using RegionAnalysisMap = std::map<RegionKey, RegionAnalysis>;

struct SampleDataset {
    SampleOrigin origin_;
    AnalysisRole role_;
    mutable ROOT::RDF::RNode dataframe_;
};

struct SampleDatasetGroup {
    SampleDataset nominal_;
    std::map<SampleVariation, SampleDataset> variations_;
};

using SampleDatasetGroupMap = std::map<SampleKey, SampleDatasetGroup>;

} // namespace analysis

#endif
