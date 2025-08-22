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
        const SampleKey&   sample_key,
        BookHistFn         book_fn,
        SystematicFutures& futures
    ) override {
        for (unsigned u = 0; u < n_universes_; ++u) {
            SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            std::string col_name = vector_name_ + "[" + std::to_string(u) + "]";
            futures.variations[uni_key][sample_key] = book_fn(col_name);
        }
    }

    TMatrixDSym computeCovariance(
        VariableResult&  result,
        SystematicFutures&     futures
    ) override {
        const auto& nominal_hist = result.total_mc_hist_;
        const auto& binning = result.binning_;
        int n = nominal_hist.nBins();
        TMatrixDSym cov(n);
        cov.Zero();

        std::vector<BinnedHistogram> varied_hists;
        for (unsigned u = 0; u < n_universes_; ++u) {
            SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            if (!futures.variations.count(uni_key)) continue;

            BinnedHistogram h_universe(binning, std::vector<double>(n, 0.0), cov);
            for(const auto& [sample_key, future] : futures.variations.at(uni_key)) {
                if (future.IsValid())
                    h_universe = h_universe + BinnedHistogram::createFromTH1D(binning, *future.GetPtr());
            }
            varied_hists.push_back(h_universe);
        }

        result.universe_projected_hists_[SystematicKey{identifier_}] = varied_hists;

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

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(
        const BinDefinition& bin,
        SystematicFutures& futures
    ) override {
        std::map<SystematicKey, BinnedHistogram> out;
        return out;
    }

private:
    std::string   identifier_;
    std::string   vector_name_;
    unsigned      n_universes_;
};

}

#endif