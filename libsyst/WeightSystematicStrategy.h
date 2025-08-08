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
        const std::vector<int>& keys,
        BookHistFn              book_fn,
        SystematicFutures&      futures
    ) override {
        for (auto key : keys) {
            futures.variations["weight"][identifier_ + "_up"][key] = book_fn(key, up_column_);
            futures.variations["weight"][identifier_ + "_dn"][key] = book_fn(key, dn_column_);
        }
    }

    TMatrixDSym computeCovariance(
        int                    key,
        const BinnedHistogram& nominal_hist,
        SystematicFutures&     futures
    ) override {
        auto& hu_future = futures.variations["weight"][identifier_ + "_up"].at(key);
        auto& hd_future = futures.variations["weight"][identifier_ + "_dn"].at(key);

        BinnedHistogram hu = BinnedHistogram::createFromTH1D(nominal_hist.bins, *hu_future.GetPtr());
        BinnedHistogram hd = BinnedHistogram::createFromTH1D(nominal_hist.bins, *hd_future.GetPtr());

        int n = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();

        for (int i = 0; i < n; ++i) {
            double d = 0.5 * (hu.getBinContent(i) - hd.getBinContent(i));
            cov(i, i) = d * d;
        }
        return cov;
    }

    std::map<std::string, BinnedHistogram> getVariedHistograms(
        int key,
        const BinDefinition& bin,
        SystematicFutures& futures
    ) override {
        std::map<std::string, BinnedHistogram> out;
        out["up"] = BinnedHistogram::createFromTH1D(bin, *futures.variations["weight"][identifier_ + "_up"].at(key).GetPtr());
        out["dn"] = BinnedHistogram::createFromTH1D(bin, *futures.variations["weight"][identifier_ + "_dn"].at(key).GetPtr());
        return out;
    }

private:
    std::string                   identifier_;
    std::string                   up_column_;
    std::string                   dn_column_;
};

}

#endif