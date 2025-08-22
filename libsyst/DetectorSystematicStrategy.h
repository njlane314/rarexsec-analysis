#ifndef DETECTOR_SYSTEMATIC_STRATEGY_H
#define DETECTOR_SYSTEMATIC_STRATEGY_H

#include "SystematicStrategy.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

class DetectorSystematicStrategy : public SystematicStrategy {
public:
    DetectorSystematicStrategy() : identifier_("detector_variation") {}

    const std::string& getName() const override { return identifier_; }

    void bookVariations(const SampleKey&, BookHistFn, SystematicFutures&) override {}

    TMatrixDSym computeCovariance(
        VariableResult&  result,
        SystematicFutures&
    ) override {
        const auto& nominal_hist = result.total_mc_hist_;
        int n_bins = nominal_hist.getNumberOfBins();
        TMatrixDSym total_detvar_cov(n_bins);
        total_detvar_cov.Zero();

        std::map<SampleVariation, BinnedHistogram> total_detvar_hists;
        for (const auto& [sample_key, var_map] : result.raw_detvar_hists_) {
            for (const auto& [var_type, hist] : var_map) {
                if (total_detvar_hists.find(var_type) == total_detvar_hists.end()) {
                    total_detvar_hists[var_type] = hist;
                } else {
                    total_detvar_hists[var_type] = total_detvar_hists[var_type] + hist;
                }
            }
        }

        auto it_cv = total_detvar_hists.find(SampleVariation::kCV);
        if (it_cv == total_detvar_hists.end()) {
            log::warn("DetectorSystematicStrategy", "No detector variation CV histogram found. Skipping.");
            return total_detvar_cov;
        }
        const auto& h_det_cv = it_cv->second;

        for (const auto& [var_key, h_det_k] : total_detvar_hists) {
            if (var_key == SampleVariation::kCV) continue;

            auto transfer_ratio = h_det_k / h_det_cv;
            auto h_proj_k = transfer_ratio * nominal_hist;
            auto delta = h_proj_k - nominal_hist;

            SystematicKey syst_key(variationToKey(var_key));
            result.transfer_ratio_hists_[syst_key] = transfer_ratio;
            result.variation_hists_[syst_key] = h_proj_k;
            result.delta_hists_[syst_key] = delta;

            TMatrixDSym cov_k(n_bins);
            for (int i = 0; i < n_bins; ++i) {
                for (int j = 0; j < n_bins; ++j) {
                    cov_k(i, j) = delta.getBinContent(i) * delta.getBinContent(j);
                }
            }
            total_detvar_cov += cov_k;
        }
        return total_detvar_cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(
        const BinningDefinition&, SystematicFutures&
    ) override {
        return {};
    }

private:
    std::string identifier_;
};

}

#endif