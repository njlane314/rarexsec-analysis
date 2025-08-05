#ifndef WEIGHT_SYSTEMATIC_STRATEGY_H
#define WEIGHT_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"

namespace analysis {

struct KnobDef {
    std::string name_;
    std::string up_column_;
    std::string dn_column_;
};

class WeightSystematicStrategy : public SystematicStrategy {
public:
    WeightSystematicStrategy(
        KnobDef     knob_def,
        BookHistFn  book_fn
    )
      : identifier_(std::move(knob_def.name_))
      , up_column_(std::move(knob_def.up_column_))
      , dn_column_(std::move(knob_def.dn_column_))
      , book_fn_(std::move(book_fn))
    {}

    const std::string& getName() const override {
        return identifier_;
    }

    void bookVariations(
        const std::vector<int>& keys,
        BookHistFn              /*book_fn*/
    ) override {
        for (auto key : keys) {
            hist_up_[key] = book_fn_(key, up_column_);
            hist_dn_[key] = book_fn_(key, dn_column_);
        }
    }

    TMatrixDSym computeCovariance(
        int              key,
        const Histogram& nominal_hist
    ) override {
        const auto& hu = hist_up_.at(key);
        const auto& hd = hist_dn_.at(key);
        int         n  = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();

        for (int i = 0; i < n; ++i) {
            double d = 0.5 * (hu.getBinContent(i) - hd.getBinContent(i));
            cov(i, i) = d * d;
        }
        return cov;
    }

    std::map<std::string, Histogram> getVariedHistograms(
        int key
    ) override {
        return {
            {"up", hist_up_.at(key)},
            {"dn", hist_dn_.at(key)}
        };
    }

private:
    std::string                   identifier_;
    std::string                   up_column_;
    std::string                   dn_column_;
    BookHistFn                    book_fn_;
    std::map<int, Histogram>      hist_up_;
    std::map<int, Histogram>      hist_dn_;
};

} 

#endif // WEIGHT_SYSTEMATIC_STRATEGY_H