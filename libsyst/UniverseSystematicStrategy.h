#ifndef UNIVERSE_SYSTEMATIC_STRATEGY_H
#define UNIVERSE_SYSTEMATIC_STRATEGY_H

#include <map>
#include <string>
#include <utility>
#include <vector>

#include <TMatrixDSym.h>

#include "Logger.h"
#include "BinnedHistogram.h"
#include "SystematicStrategy.h"

namespace analysis {

class UniverseSystematicStrategy : public SystematicStrategy {
  public:
    UniverseSystematicStrategy(UniverseDef universe_def, bool store_universe_hists = false)
        : identifier_(std::move(universe_def.name_)),
          vector_name_(std::move(universe_def.vector_name_)),
          n_universes_(universe_def.n_universes_),
          store_universe_hists_(store_universe_hists) {}

    const std::string &getName() const override { return identifier_; }

    void bookVariations(const SampleKey &sample_key, ROOT::RDF::RNode &rnode, const BinningDefinition &binning,
                        const ROOT::RDF::TH1DModel &model, SystematicFutures &futures) override {
        log::debug("UniverseSystematicStrategy::bookVariations", identifier_, "sample", sample_key.str(),
                   "universes", n_universes_);
        for (unsigned u = 0; u < n_universes_; ++u) {
            const SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            const auto weight = [u](const ROOT::RVec<unsigned short> &weights) {
                if (u < weights.size()) return static_cast<double>(weights[u]);
                return 1.0;
            };

            const auto uni_weight_name = "_uni_w_" + std::to_string(u);
            auto node = rnode.Define(uni_weight_name, weight, {vector_name_});
            futures.variations[uni_key][sample_key] =
                node.Histo1D(model, binning.getVariable(), uni_weight_name);
        }
    }

    TMatrixDSym computeCovariance(VariableResult &result, SystematicFutures &futures) override {
        const auto &nominal_hist = result.total_mc_hist_;
        const auto &binning = result.binning_;
        const int n = nominal_hist.getNumberOfBins();
        TMatrixDSym cov(n);
        cov.Zero();

        std::vector<BinnedHistogram> stored_hists;
        log::debug("UniverseSystematicStrategy::computeCovariance", identifier_, "processing", n_universes_,
                   "universes");
        unsigned processed_universes = 0;
        for (unsigned u = 0; u < n_universes_; ++u) {
            const SystematicKey uni_key(identifier_ + "_u" + std::to_string(u));
            if (!futures.variations.count(uni_key)) {
                log::warn("UniverseSystematicStrategy::computeCovariance", "Missing universe", u, "for", identifier_);
                continue;
            }

            auto h_universe = buildUniverseHistogram(binning, n, uni_key, futures);
            updateCovarianceMatrix(cov, nominal_hist, h_universe);

            ++processed_universes;
            storeUniverseHistogram(stored_hists, std::move(h_universe));
        }

        const double n_universes = static_cast<double>(processed_universes);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                const double val = n_universes == 0 ? 0 : cov(i, j) / n_universes;
                cov(i, j) = val;
                cov(j, i) = val;
            }
        }

        if (store_universe_hists_ && !stored_hists.empty()) {
            result.universe_projected_hists_[SystematicKey{identifier_}] = std::move(stored_hists);
        }

        log::debug("UniverseSystematicStrategy::computeCovariance", identifier_, "covariance calculated with",
                   processed_universes, "universes");
        return cov;
    }

    std::map<SystematicKey, BinnedHistogram> getVariedHistograms(const BinningDefinition &,
                                                                 SystematicFutures &) override {
        std::map<SystematicKey, BinnedHistogram> out;
        return out;
    }

  private:
    BinnedHistogram buildUniverseHistogram(const BinningDefinition &binning, int n, const SystematicKey &key,
                                           SystematicFutures &futures) {
        Eigen::VectorXd shifts = Eigen::VectorXd::Zero(n);
        Eigen::MatrixXd shifts_mat = shifts;
        BinnedHistogram h_universe(binning, std::vector<double>(n, 0.0), shifts_mat);

        for (auto &[sample_key, future] : futures.variations.at(key)) {
            if (future.GetPtr()) {
                h_universe = h_universe + BinnedHistogram::createFromTH1D(binning, *future.GetPtr());
            }
        }
        return h_universe;
    }

    void updateCovarianceMatrix(TMatrixDSym &cov, const BinnedHistogram &nominal_hist,
                                const BinnedHistogram &h_universe) {
        const int n = nominal_hist.getNumberOfBins();
        for (int i = 0; i < n; ++i) {
            const double di = h_universe.getBinContent(i) - nominal_hist.getBinContent(i);
            for (int j = 0; j <= i; ++j) {
                const double dj = h_universe.getBinContent(j) - nominal_hist.getBinContent(j);
                cov(i, j) += di * dj;
            }
        }
    }

    void storeUniverseHistogram(std::vector<BinnedHistogram> &stored_hists, BinnedHistogram &&h_universe) {
        if (store_universe_hists_) stored_hists.push_back(std::move(h_universe));
    }

    std::string identifier_;
    std::string vector_name_;
    unsigned n_universes_;
    bool store_universe_hists_;
};

}

#endif
