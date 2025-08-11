#ifndef UNIVERSE_SYSTEMATIC_STRATEGY_H
#define UNIVERSE_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"
#include "Logger.h"

namespace analysis {

class UniverseSystematicStrategy : public SystematicStrategy {
public:
    UniverseSystematicStrategy(
        UniverseDef   universe_def
    )
      : identifier_(std::move(universe_def.name_))
      , vector_name_(std::move(universe_def.vector_name_))
      , n_universes_(universe_def.n_universes_)
    {}

    const std::string& getName() const override {
        return identifier_;
    }

    void bookVariations(
        const std::vector<int>& keys,
        BookHistFn              book_fn,
        SystematicFutures&      futures
    ) override {
        log::debug("UniverseSystematicStrategy::bookVariations", "Booking variations for ", identifier_);
        for (auto key : keys) {
            for (unsigned u = 0; u < n_universes_; ++u) {
                std::string col_name = vector_name_ + "[" + std::to_string(u) + "]";
                log::debug("UniverseSystematicStrategy::bookVariations", "Booking universe ", u, " with column expression: ", col_name);
                futures.variations["universe"][identifier_ + "_u" + std::to_string(u)][key] = book_fn(key, col_name);
            }
        }
    }

    TMatrixDSym computeCovariance(
        int                    key,
        const BinnedHistogram& nominal_hist,
        SystematicFutures&     futures
    ) override {
        int n = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();

        std::vector<BinnedHistogram> varied_hists;
        for (unsigned u = 0; u < n_universes_; ++u) {
            auto& future = futures.variations["universe"][identifier_ + "_u" + std::to_string(u)].at(key);
            varied_hists.push_back(BinnedHistogram::createFromTH1D(nominal_hist.bins, *future.GetPtr()));
        }

        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                double sum = 0;
                for (const auto& hist : varied_hists) {
                    double di = hist.getBinContent(i) - nominal_hist.getBinContent(i);
                    double dj = hist.getBinContent(j) - nominal_hist.getBinContent(j);
                    sum += di * dj;
                }
                double val = varied_hists.empty() ? 0 : sum / varied_hists.size();
                cov(i, j) = val;
                cov(j, i) = val;
            }
        }
        return cov;
    }

    std::map<std::string, BinnedHistogram> getVariedHistograms(
        int key,
        const BinDefinition& bin,
        SystematicFutures& futures
    ) override {
        std::map<std::string, BinnedHistogram> out;
        for (unsigned u = 0; u < n_universes_; ++u) {
            auto& future = futures.variations["universe"][identifier_ + "_u" + std::to_string(u)].at(key);
            out["u" + std::to_string(u)] = BinnedHistogram::createFromTH1D(bin, *future.GetPtr());
        }
        return out;
    }

private:
    std::string                                     identifier_;
    std::string                                     vector_name_;
    unsigned                                        n_universes_;
};

}

#endif