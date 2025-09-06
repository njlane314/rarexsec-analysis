#ifndef DETECTOR_SYSTEMATIC_STRATEGY_H
#define DETECTOR_SYSTEMATIC_STRATEGY_H

#include "AnalysisLogger.h"
#include "BinnedHistogram.h"
#include "SystematicStrategy.h"
#include <cmath>

namespace analysis {

class DetectorSystematicStrategy : public SystematicStrategy {
  public:
    DetectorSystematicStrategy() : identifier_("detector_variation") {}

    const std::string &getName() const override { return identifier_; }

    void bookVariations(const SampleKey &, ROOT::RDF::RNode &, const BinningDefinition &, const ROOT::RDF::TH1DModel &,
                        SystematicFutures &) override {}

    TMatrixDSym computeCovariance(VariableResult &result, SystematicFutures &) override {
        const auto &nominal_hist = result.total_mc_hist_;
        int n_bins = nominal_hist.getNumberOfBins();
        TMatrixDSym total_detvar_cov(n_bins);
        total_detvar_cov.Zero();

        log::debug("DetectorSystematicStrategy::computeCovariance",
                   "Raw detvar histograms:", result.raw_detvar_hists_.size());
        if (result.raw_detvar_hists_.empty()) {
            log::info("DetectorSystematicStrategy::computeCovariance",
                      "No detector variation samples found. Skipping detector "
                      "systematics.");
            return total_detvar_cov;
        }

        std::map<SampleVariation, BinnedHistogram> total_detvar_hists;
        for (const auto &[sample_key, var_map] : result.raw_detvar_hists_) {
            log::debug("DetectorSystematicStrategy::computeCovariance", "Aggregating sample", sample_key.str());
            for (const auto &[var_type, hist] : var_map) {
                log::debug("DetectorSystematicStrategy::computeCovariance", "--> variation", variationToKey(var_type));
                if (total_detvar_hists.find(var_type) == total_detvar_hists.end()) {
                    total_detvar_hists[var_type] = hist;
                } else {
                    total_detvar_hists[var_type] = total_detvar_hists[var_type] + hist;
                }
            }
        }

        auto it_cv = total_detvar_hists.find(SampleVariation::kCV);
        if (it_cv == total_detvar_hists.end()) {
            log::warn("DetectorSystematicStrategy::computeCovariance",
                      "No detector variation CV histogram found. Skipping.");
            return total_detvar_cov;
        }
        auto h_det_cv = it_cv->second;
        int n_cv_bins = h_det_cv.getNumberOfBins();
        for (int i = 0; i < n_cv_bins; ++i) {
            double cv = h_det_cv.getBinContent(i);
            if (!std::isfinite(cv) || cv == 0.0) {
                h_det_cv.hist.counts[i] = 0.0;
                h_det_cv.hist.shifts.row(i).setZero();
            }
        }

        for (const auto &[var_key, h_det_k] : total_detvar_hists) {
            if (var_key == SampleVariation::kCV)
                continue;

            log::debug("DetectorSystematicStrategy::computeCovariance", "Projecting variation",
                       variationToKey(var_key));

            auto transfer_ratio = h_det_k / h_det_cv;
            int n_tr_bins = transfer_ratio.getNumberOfBins();
            for (int i = 0; i < n_tr_bins; ++i) {
                if (!std::isfinite(transfer_ratio.getBinContent(i))) {
                    transfer_ratio.hist.counts[i] = 0.0;
                    transfer_ratio.hist.shifts.row(i).setZero();
                }
            }
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
        log::debug("DetectorSystematicStrategy::computeCovariance", "Computed detector covariance with",
                   total_detvar_hists.size() - 1, "variations");
        return total_detvar_cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(const BinningDefinition &,
                                                                 SystematicFutures &) override {
        return {};
    }

  private:
    std::string identifier_;
};

} // namespace analysis

#endif
