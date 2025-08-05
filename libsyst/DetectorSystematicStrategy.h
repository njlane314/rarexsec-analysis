#ifndef DETECTOR_SYSTEMATIC_POLICY_H
#define DETECTOR_SYSTEMATIC_POLICY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"

namespace analysis {

class DetectorSystematicStrategy : public SystematicStrategy {
public:
    DetectorSystematicStrategy(
        std::string policy_name,
        BookHistFn  book_fn
    )
      : policy_name_(std::move(policy_name))
      , book_fn_(std::move(book_fn))
    {}

    const std::string& name() const override {
        return policy_name_;
    }

    void bookVariations(
        const std::vector<int>& keys,
        BookHistFn              /*book_fn*/
    ) override {
        for (auto key : keys) {
            hist_var_[key] = book_fn_(key, "central_value_weight");
        }
    }

    TMatrixDSym computeCovariance(
        int              key,
        const Histogram& nominal_hist
    ) override {
        const auto& hist = hist_var_.at(key);
        int n = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();
        for (int i = 0; i < n; ++i) {
            double diff = hist.getBinContent(i) - nominal_hist.getBinContent(i);
            cov(i, i) = diff * diff;
        }
        return cov;
    }

    std::map<std::string, Histogram> variedHistograms(
        int key
    ) override {
        return { {"var", hist_var_.at(key)} };
    }

private:
    std::string                policy_name_;
    BookHistFn                 book_fn_;
    std::map<int, Histogram>   hist_var_;
};

} 

#endif // DETECTOR_SYSTEMATIC_POLICY_H