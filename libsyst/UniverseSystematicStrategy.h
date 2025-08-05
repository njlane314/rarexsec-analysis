#ifndef UNIVERSE_SYSTEMATIC_STRATEGY_H
#define UNIVERSE_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"

namespace analysis {

class UniverseSystematicStrategy : public SystematicStrategy {
public:
    UniverseSystematicStrategy(
        UniverseDef   universe_def,
        BookHistFn    book_fn
    )
      : identifier_(std::move(universe_def.name_))
      , vector_name_(std::move(universe_def.vector_name_))
      , n_universes_(universe_def.n_universes_)
      , book_fn_(std::move(book_fn))
    {}

    const std::string& getName() const override {
        return identifier_;
    }

    void bookVariations(
        const std::vector<int>& keys,
        BookHistFn              book_fn
    ) override {
        for (auto key : keys) {
            auto& univ_map = hist_universes_[key];
            for (unsigned u = 0; u < n_universes_; ++u) {
                std::string col = vector_name_ + "[" + std::to_string(u) + "]";
                univ_map.emplace(u, book_fn(key, col));
            }
        }
    }

    TMatrixDSym computeCovariance(
        int              key,
        const BinnedHistogram& nominal_hist
    ) override {
        auto& univ_map = hist_universes_.at(key);
        int         n  = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                double sum = 0;
                for (auto& [u, hist] : univ_map) {
                    double di = hist.getBinContent(i) - nominal_hist.getBinContent(i);
                    double dj = hist.getBinContent(j) - nominal_hist.getBinContent(j);
                    sum += di * dj;
                }
                double val = sum / univ_map.size();
                cov(i, j) = val;
                cov(j, i) = val;
            }
        }
        return cov;
    }

    std::map<std::string, BinnedHistogram> getVariedHistograms(
        int key
    ) override {
        std::map<std::string, BinnedHistogram> out;
        for (auto& [u, hist] : hist_universes_.at(key)) {
            out["u" + std::to_string(u)] = hist;
        }
        return out;
    }

private:
    std::string                                     identifier_;
    std::string                                     vector_name_;
    unsigned                                        n_universes_;
    BookHistFn                                      book_fn_;
    std::map<int, std::map<unsigned, BinnedHistogram>>    hist_universes_;
};

}

#endif