#ifndef NORMALISATION_SYSTEMATIC_STRATEGY_H
#define NORMALISATION_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"

namespace analysis {

class NormalisationSystematicStrategy : public SystematicStrategy {
public:
    NormalisationSystematicStrategy(
        std::string identifier,
        double      fraction
    )
      : identifier_(std::move(identifier))
      , fraction_(fraction)
    {}

    const std::string& getName() const override {
        return identifier_;
    }

    void bookVariations(
        const std::vector<int>&,
        BookHistFn,
        SystematicFutures&
    ) override {}

    TMatrixDSym computeCovariance(
        int,
        const BinnedHistogram& nominal_hist,
        SystematicFutures&
    ) override {
        int n = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                double v_i = nominal_hist.getBinContent(i);
                double v_j = nominal_hist.getBinContent(j);
                cov(i, j) = (fraction_ * v_i) * (fraction_ * v_j);
            }
        }
        return cov;
    }

    std::map<std::string, BinnedHistogram> getVariedHistograms(
        int,
        const BinDefinition&,
        SystematicFutures&
    ) override {
        return {};
    }

private:
    std::string identifier_;
    double      fraction_;
};

}

#endif