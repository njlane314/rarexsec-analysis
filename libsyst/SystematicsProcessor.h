#ifndef SYSTEMATICS_PROCESSOR_H
#define SYSTEMATICS_PROCESSOR_H

#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <TMatrixDSym.h>

#include "AnalysisLogger.h"
#include "BinnedHistogram.h"
#include "DetectorSystematicStrategy.h"
#include "SystematicStrategy.h"
#include "UniverseSystematicStrategy.h"
#include "VariableRegistry.h"
#include "WeightSystematicStrategy.h"

namespace analysis {

class SystematicsProcessor {
  public:
    SystematicsProcessor(const VariableRegistry &registry, bool store_universe_hists = false)
        : SystematicsProcessor(createKnobs(registry), createUniverses(registry), store_universe_hists) {}

    SystematicsProcessor(std::vector<KnobDef> knob_definitions, std::vector<UniverseDef> universe_definitions,
                         bool store_universe_hists = false)
        : knob_definitions_(std::move(knob_definitions)), universe_definitions_(std::move(universe_definitions)),
          store_universe_hists_(store_universe_hists) {
        systematic_strategies_.emplace_back(std::make_unique<DetectorSystematicStrategy>());
        for (const auto &knob : knob_definitions_) {
            systematic_strategies_.emplace_back(std::make_unique<WeightSystematicStrategy>(knob));
        }
        for (const auto &universe : universe_definitions_) {
            systematic_strategies_.emplace_back(
                std::make_unique<UniverseSystematicStrategy>(universe, store_universe_hists_));
        }

        log::debug("SystematicsProcessor", "Initialised with", knob_definitions_.size(), "weight knobs and",
                   universe_definitions_.size(), "universe variations");
    }

    void bookSystematics(const SampleKey &sample_key, ROOT::RDF::RNode &rnode, const BinningDefinition &binning,
                         const ROOT::RDF::TH1DModel &model) {
        log::debug("SystematicsProcessor::bookSystematics", "Booking variations for sample", sample_key.str());
        for (const auto &strategy : systematic_strategies_) {
            log::debug("SystematicsProcessor::bookSystematics", "-> Strategy", strategy->getName());
            strategy->bookVariations(sample_key, rnode, binning, model, systematic_futures_);
        }
        log::debug("SystematicsProcessor::bookSystematics", "Completed booking for sample", sample_key.str());
    }

    void processSystematics(VariableResult &result) {
        log::debug("SystematicsProcessor::processSystematics", "Commencing covariance calculations");
        for (const auto &strategy : systematic_strategies_) {
            SystematicKey key{strategy->getName()};
            log::debug("SystematicsProcessor::processSystematics", "Computing covariance for", key.str());
            auto cov = strategy->computeCovariance(result, systematic_futures_);
            sanitizeMatrix(cov);
            log::debug("SystematicsProcessor::processSystematics", key.str(), "matrix size", cov.GetNrows(), "x",
                       cov.GetNcols());
            result.covariance_matrices_.insert_or_assign(key, cov);
        }
        combineCovariances(result);
        log::debug("SystematicsProcessor::processSystematics", "Covariance calculation complete");
    }

    void clearFutures() { systematic_futures_.variations.clear(); }

  private:
    static void sanitizeMatrix(TMatrixDSym &m) {
        for (int i = 0; i < m.GetNrows(); ++i) {
            for (int j = 0; j < m.GetNcols(); ++j) {
                if (!std::isfinite(m(i, j))) m(i, j) = 0.0;
            }
        }
    }

    static void combineCovariances(VariableResult &result) {
        int n_bins = result.total_mc_hist_.getNumberOfBins();

        if (n_bins <= 0) return;

        result.total_covariance_.ResizeTo(n_bins, n_bins);
        result.total_covariance_ = result.total_mc_hist_.hist.covariance();

        log::debug("SystematicsProcessor::combineCovariances", "Combining covariance matrices");

        for (const auto &[name, cov_matrix] : result.covariance_matrices_) {
            if (cov_matrix.GetNrows() == n_bins) {
                TMatrixDSym cov = cov_matrix;
                sanitizeMatrix(cov);

                log::debug("SystematicsProcessor::combineCovariances", "Adding matrix", name.str());

                result.total_covariance_ += cov;
            } else {
                log::warn("SystematicsProcessor::combineCovariances", "Skipping systematic", name.str(),
                          "due to incompatible matrix size (", cov_matrix.GetNrows(), "x", cov_matrix.GetNcols(),
                          "vs expected", n_bins, "x", n_bins, ")");
            }
        }

        sanitizeMatrix(result.total_covariance_);

        result.nominal_with_band_ = result.total_mc_hist_;
        result.nominal_with_band_.hist.shifts.resize(n_bins, 0);
        result.nominal_with_band_.addCovariance(result.total_covariance_);
    }

    static std::vector<KnobDef> createKnobs(const VariableRegistry &registry) {
        std::vector<KnobDef> out;
        out.reserve(registry.knobVariations().size());
        for (auto &kv : registry.knobVariations()) {
            out.push_back({kv.first, kv.second.first, kv.second.second});
        }
        return out;
    }

    static std::vector<UniverseDef> createUniverses(const VariableRegistry &registry) {
        std::vector<UniverseDef> out;
        out.reserve(registry.multiUniverseVariations().size());
        for (auto &kv : registry.multiUniverseVariations()) {
            out.push_back({kv.first, kv.first, kv.second});
        }
        return out;
    }

    std::vector<std::unique_ptr<SystematicStrategy>> systematic_strategies_;
    std::vector<KnobDef> knob_definitions_;
    std::vector<UniverseDef> universe_definitions_;
    bool store_universe_hists_;
    SystematicFutures systematic_futures_;
};

} // namespace analysis

#endif
