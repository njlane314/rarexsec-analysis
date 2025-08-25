#ifndef WEIGHT_SYSTEMATIC_STRATEGY_H
#define WEIGHT_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"

namespace analysis {

class WeightSystematicStrategy : public SystematicStrategy {
public:
    WeightSystematicStrategy(
        KnobDef     knob_def
    )
      : identifier_(std::move(knob_def.name_))
      , up_column_(std::move(knob_def.up_column_))
      , dn_column_(std::move(knob_def.dn_column_))
    {}

    const std::string& getName() const override {
        return identifier_;
    }

    void bookVariations(
        const SampleKey& sample_key,
        ROOT::RDF::RNode& rnode,
        const BinningDefinition& binning,
        const ROOT::RDF::TH1DModel& model,
        SystematicFutures& futures
    ) override {
        SystematicKey up_key{identifier_ + "_up"};
        SystematicKey dn_key{identifier_ + "_dn"};
        futures.variations[up_key][sample_key] = rnode.Histo1D(model, binning.getVariable(), up_column_);
        futures.variations[dn_key][sample_key] = rnode.Histo1D(model, binning.getVariable(), dn_column_);
    }

    TMatrixDSym computeCovariance(
        VariableResult&  result,
        SystematicFutures&     futures
    ) override {
        const auto& nominal_hist = result.total_mc_hist_;
        const auto& binning = result.binning_;
        int n = nominal_hist.getNumberOfBins();
        TMatrixDSym cov(n);
        cov.Zero();

        Eigen::VectorXd shifts = Eigen::VectorXd::Zero(n);
        Eigen::MatrixXd shifts_mat = shifts;
        BinnedHistogram hu(binning, std::vector<double>(n, 0.0), shifts_mat);
        BinnedHistogram hd(binning, std::vector<double>(n, 0.0), shifts_mat);

        SystematicKey up_key{identifier_ + "_up"};
        SystematicKey dn_key{identifier_ + "_dn"};

        if (futures.variations.count(up_key)) {
            for(auto& [sample_key, future] : futures.variations.at(up_key)){
                if (future.GetPtr())
                    hu = hu + BinnedHistogram::createFromTH1D(binning, *future.GetPtr());
            }
        }
        if (futures.variations.count(dn_key)) {
            for(auto& [sample_key, future] : futures.variations.at(dn_key)){
                if (future.GetPtr())
                    hd = hd + BinnedHistogram::createFromTH1D(binning, *future.GetPtr());
            }
        }

        result.variation_hists_[up_key] = hu;
        result.variation_hists_[dn_key] = hd;

        for (int i = 0; i < n; ++i) {
            double d = 0.5 * (hu.getBinContent(i) - hd.getBinContent(i));
            cov(i, i) = d * d;
        }
        return cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(
        const BinningDefinition&,
        SystematicFutures&
    ) override {
        std::map<SystematicKey, BinnedHistogram> out;
        return out;
    }

private:
    std::string                   identifier_;
    std::string                   up_column_;
    std::string                   dn_column_;
};

}

#endif