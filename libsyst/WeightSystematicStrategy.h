#ifndef WEIGHT_SYSTEMATIC_STRATEGY_H
#define WEIGHT_SYSTEMATIC_STRATEGY_H

#include "BinnedHistogram.h"
#include "SystematicStrategy.h"
#include <TMatrixDSym.h>
#include <map>
#include <string>
#include <vector>

namespace analysis {

class WeightSystematicStrategy : public SystematicStrategy {
  public:
    WeightSystematicStrategy(KnobDef knob_def)
        : identifier_(std::move(knob_def.name_)),
          up_column_(std::move(knob_def.up_column_)),
          dn_column_(std::move(knob_def.dn_column_)) {}

    const std::string &getName() const override { return identifier_; }

    void bookVariations(const SampleKey &sample_key, ROOT::RDF::RNode &rnode, const BinningDefinition &binning,
                        const ROOT::RDF::TH1DModel &model, SystematicFutures &futures) override {
        log::debug("WeightSystematicStrategy::bookVariations", identifier_, "sample", sample_key.str());
        const SystematicKey up_key{identifier_ + "_up"};
        const SystematicKey dn_key{identifier_ + "_dn"};
        futures.variations[up_key][sample_key] = rnode.Histo1D(model, binning.getVariable(), up_column_);
        futures.variations[dn_key][sample_key] = rnode.Histo1D(model, binning.getVariable(), dn_column_);
    }

    TMatrixDSym computeCovariance(VariableResult &result, SystematicFutures &futures) override {
        const auto &nominal_hist = result.total_mc_hist_;
        const auto &binning = result.binning_;
        const int n = nominal_hist.getNumberOfBins();
        TMatrixDSym cov(n);
        cov.Zero();

        const SystematicKey up_key{identifier_ + "_up"};
        const SystematicKey dn_key{identifier_ + "_dn"};

        const auto hu = accumulateVariation(binning, n, up_key, futures, "up");
        const auto hd = accumulateVariation(binning, n, dn_key, futures, "down");

        result.variation_hists_[up_key] = hu;
        result.variation_hists_[dn_key] = hd;

        std::vector<double> diff(n);
        for (int i = 0; i < n; ++i) {
            diff[i] = 0.5 * (hu.getBinContent(i) - hd.getBinContent(i));
            for (int j = 0; j <= i; ++j) {
                const double val = diff[i] * diff[j];
                cov(i, j) = val;
                cov(j, i) = val;
            }
        }
        log::debug("WeightSystematicStrategy::computeCovariance", identifier_, "covariance calculated");
        return cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(const BinningDefinition &,
                                                                 SystematicFutures &) override {
        std::map<SystematicKey, BinnedHistogram> out;
        return out;
    }

  private:
    BinnedHistogram accumulateVariation(const BinningDefinition &binning, int n, const SystematicKey &key,
                                        SystematicFutures &futures, const std::string &direction) const {
        Eigen::VectorXd shifts = Eigen::VectorXd::Zero(n);
        Eigen::MatrixXd shifts_mat = shifts;
        BinnedHistogram hist(binning, std::vector<double>(n, 0.0), shifts_mat);
        if (!futures.variations.count(key)) {
            log::warn("WeightSystematicStrategy::computeCovariance", "Missing", direction, "variation for",
                      identifier_);
            return hist;
        }

        log::debug("WeightSystematicStrategy::computeCovariance", "Accumulating", direction, "variations for",
                   identifier_);
        for (auto &[sample_key, future] : futures.variations.at(key)) {
            if (future.GetPtr()) {
                hist = hist + BinnedHistogram::createFromTH1D(binning, *future.GetPtr());
            }
        }
        return hist;
    }

    std::string identifier_;
    std::string up_column_;
    std::string dn_column_;
};

}

#endif
