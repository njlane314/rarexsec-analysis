#ifndef DETECTOR_SYSTEMATIC_STRATEGY_H
#define DETECTOR_SYSTEMATIC_STRATEGY_H

#include "Logger.h"
#include "BinnedHistogram.h"
#include "SystematicStrategy.h"
#include <cmath>
#include <optional>

namespace analysis {

class DetectorSystematicStrategy : public SystematicStrategy {
  public:
    DetectorSystematicStrategy() : identifier_("detector_variation") {}

    const std::string &getName() const override { return identifier_; }

    void bookVariations(const SampleKey &, ROOT::RDF::RNode &, const BinningDefinition &, const ROOT::RDF::TH1DModel &,
                        SystematicFutures &) override {}

    TMatrixDSym computeCovariance(VariableResult &result, SystematicFutures &) override {
        const auto &nominal_hist = result.total_mc_hist_;
        const int n_bins = nominal_hist.getNumberOfBins();

        TMatrixDSym total_detvar_cov(n_bins);
        total_detvar_cov.Zero();

        log::debug("DetectorSystematicStrategy::computeCovariance", "Raw detvar histograms:",
                   result.raw_detvar_hists_.size());

        if (result.raw_detvar_hists_.empty()) {
            log::info("DetectorSystematicStrategy::computeCovariance",
                      "No detector variation samples found. Skipping detector systematics.");
            return total_detvar_cov;
        }

        auto total_detvar_hists = aggregateVariations(result);
        auto h_det_cv_opt = sanitiseCvHistogram(total_detvar_hists);
        if (!h_det_cv_opt) return total_detvar_cov;

        auto h_det_cv = *h_det_cv_opt;
        projectVariations(result, nominal_hist, h_det_cv, total_detvar_hists);
        total_detvar_cov = accumulateCovariance(result, n_bins);

        log::debug("DetectorSystematicStrategy::computeCovariance", "Computed detector covariance with",
                   total_detvar_hists.size() - 1, "variations");
        return total_detvar_cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(const BinningDefinition &,
                                                                 SystematicFutures &) override {
        return {};
    }

  private:
    std::map<SampleVariation, BinnedHistogram> aggregateVariations(const VariableResult &result) {
        std::map<SampleVariation, BinnedHistogram> total_detvar_hists;

        for (const auto &[sample_key, variations] : result.raw_detvar_hists_) {
            log::debug("DetectorSystematicStrategy::computeCovariance", "Aggregating sample", sample_key.str());
            for (const auto &[variation, hist] : variations) {
                log::debug("DetectorSystematicStrategy::computeCovariance", "--> variation",
                           variationToKey(variation));
                auto [it, inserted] = total_detvar_hists.try_emplace(variation, hist);
                if (!inserted) it->second = it->second + hist;
            }
        }
        return total_detvar_hists;
    }

    std::optional<BinnedHistogram> sanitiseCvHistogram(std::map<SampleVariation, BinnedHistogram> &total_detvar_hists) {
        auto it = total_detvar_hists.find(SampleVariation::kCV);
        if (it == total_detvar_hists.end()) {
            log::warn("DetectorSystematicStrategy::computeCovariance",
                      "No detector variation CV histogram found. Skipping.");
            return std::nullopt;
        }

        auto h_det_cv = it->second;
        const int n_cv_bins = h_det_cv.getNumberOfBins();
        for (int i = 0; i < n_cv_bins; ++i) {
            const double cv = h_det_cv.getBinContent(i);
            if (!std::isfinite(cv) || cv == 0.0) {
                h_det_cv.hist.counts[i] = 0.0;
                h_det_cv.hist.shifts.row(i).setZero();
            }
        }

        it->second = h_det_cv;
        return h_det_cv;
    }

    void projectVariations(VariableResult &result, const BinnedHistogram &nominal_hist, const BinnedHistogram &h_det_cv,
                           const std::map<SampleVariation, BinnedHistogram> &total_detvar_hists) {
        for (const auto &[var_key, h_det_k] : total_detvar_hists) {
            if (var_key == SampleVariation::kCV) continue;

            log::debug("DetectorSystematicStrategy::computeCovariance", "Projecting variation",
                       variationToKey(var_key));

            auto transfer_ratio = h_det_k / h_det_cv;
            const int n_tr_bins = transfer_ratio.getNumberOfBins();
            for (int i = 0; i < n_tr_bins; ++i) {
                if (!std::isfinite(transfer_ratio.getBinContent(i))) {
                    transfer_ratio.hist.counts[i] = 0.0;
                    transfer_ratio.hist.shifts.row(i).setZero();
                }
            }

            const auto h_proj_k = transfer_ratio * nominal_hist;
            const auto delta = h_proj_k - nominal_hist;
            const SystematicKey syst_key(variationToKey(var_key));
            result.transfer_ratio_hists_[syst_key] = transfer_ratio;
            result.variation_hists_[syst_key] = h_proj_k;
            result.delta_hists_[syst_key] = delta;
        }
    }

    TMatrixDSym accumulateCovariance(const VariableResult &result, int n_bins) {
        TMatrixDSym total_detvar_cov(n_bins);

        total_detvar_cov.Zero();

        for (const auto &[var_key, delta] : result.delta_hists_) {
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

    std::string identifier_;
};

}

#endif
