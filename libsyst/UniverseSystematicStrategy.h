#ifndef UNIVERSE_SYSTEMATIC_STRATEGY_H
#define UNIVERSE_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <utility>

#include <TMatrixDSym.h>

#include "AnalysisLogger.h"
#include "BinnedHistogram.h"
#include "SystematicStrategy.h"

namespace analysis {

class UniverseSystematicStrategy : public SystematicStrategy {
  public:
    UniverseSystematicStrategy(UniverseDef universe_def,
                               bool store_universe_hists = false)
        : identifier_(std::move(universe_def.name_)),
          vector_name_(std::move(universe_def.vector_name_)),
          n_universes_(universe_def.n_universes_),
          store_universe_hists_(store_universe_hists) {}

    const std::string &getName() const override { return identifier_; }

    void bookVariations(const SampleKey &sample_key, ROOT::RDF::RNode &rnode,
                        const BinningDefinition &binning,
                        const ROOT::RDF::TH1DModel &model,
                        SystematicFutures &futures) override {
        log::debug("UniverseSystematicStrategy::bookVariations", identifier_,
                   "sample", sample_key.str(), "universes", n_universes_);
        for (unsigned u = 0; u < n_universes_; ++u) {
            SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            std::string new_col_name = vector_name_ + "_u" + std::to_string(u);

            auto d_with_weight =
                rnode.Define(new_col_name,
                             [u, vec_name = this->vector_name_](
                                 const ROOT::RVec<unsigned short> &weights) {
                                 if (u < weights.size()) {
                                     return static_cast<double>(weights[u]);
                                 }
                                 return 1.0;
                             },
                             {vector_name_});

            futures.variations[uni_key][sample_key] = d_with_weight.Histo1D(
                model, binning.getVariable(), new_col_name);
        }
    }

    TMatrixDSym computeCovariance(VariableResult &result,
                                  SystematicFutures &futures) override {
        const auto &nominal_hist = result.total_mc_hist_;
        const auto &binning = result.binning_;
        int n = nominal_hist.getNumberOfBins();
        TMatrixDSym cov(n);
        cov.Zero();

        std::vector<BinnedHistogram> stored_hists;
        log::debug("UniverseSystematicStrategy::computeCovariance", identifier_,
                   "processing", n_universes_, "universes");
        unsigned processed_universes = 0;
        for (unsigned u = 0; u < n_universes_; ++u) {
            SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            if (!futures.variations.count(uni_key)) {
                log::warn("UniverseSystematicStrategy::computeCovariance",
                          "Missing universe", u, "for", identifier_);
                continue;
            }

            Eigen::VectorXd shifts = Eigen::VectorXd::Zero(n);
            Eigen::MatrixXd shifts_mat = shifts;
            BinnedHistogram h_universe(binning, std::vector<double>(n, 0.0),
                                       shifts_mat);
            for (auto &[sample_key, future] : futures.variations.at(uni_key)) {
                if (future.GetPtr())
                    h_universe = h_universe + BinnedHistogram::createFromTH1D(
                                                  binning, *future.GetPtr());
            }

            for (int i = 0; i < n; ++i) {
                double di =
                    h_universe.getBinContent(i) - nominal_hist.getBinContent(i);
                for (int j = 0; j <= i; ++j) {
                    double dj = h_universe.getBinContent(j) -
                                nominal_hist.getBinContent(j);
                    cov(i, j) += di * dj;
                }
            }

            ++processed_universes;
            if (store_universe_hists_) {
                stored_hists.push_back(std::move(h_universe));
            }
        }

        double n_universes = static_cast<double>(processed_universes);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                double val =
                    n_universes == 0 ? 0 : cov(i, j) / (n_universes * n_universes);
                cov(i, j) = val;
                cov(j, i) = val;
            }
        }

        if (store_universe_hists_ && !stored_hists.empty()) {
            result.universe_projected_hists_[SystematicKey{identifier_}] =
                std::move(stored_hists);
        }

        log::debug("UniverseSystematicStrategy::computeCovariance", identifier_,
                   "covariance calculated with", processed_universes,
                   "universes");
        return cov;
    }

    std::map<SystematicKey, BinnedHistogram>
    getVariedHistograms(const BinningDefinition &,
                        SystematicFutures &) override {
        std::map<SystematicKey, BinnedHistogram> out;
        return out;
    }

  private:
    std::string identifier_;
    std::string vector_name_;
    unsigned n_universes_;
    bool store_universe_hists_;
};

} // namespace analysis

#endif
