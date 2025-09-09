#ifndef SYSTEMATICS_PROCESSOR_H
#define SYSTEMATICS_PROCESSOR_H

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <mutex>

#include <TMatrixDSym.h>

#include "Logger.h"
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
        : SystematicsProcessor(SystematicsProcessor::createKnobs(registry),
                               SystematicsProcessor::createUniverses(registry),
                               store_universe_hists) {}

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
        if (!hasSystematics() && result.raw_detvar_hists_.empty()) {
            log::info("SystematicsProcessor::processSystematics",
                      "No systematics found. Skipping covariance calculation.");
            return;
        }

        log::debug("SystematicsProcessor::processSystematics", "Commencing covariance calculations");
        for (const auto &strategy : systematic_strategies_) {
            VariableResult local_result = result;
            SystematicKey key{strategy->getName()};
            log::debug("SystematicsProcessor::processSystematics", "Computing covariance for", key.str());
            auto cov = strategy->computeCovariance(local_result, systematic_futures_);
            sanitiseMatrix(cov);
            log::debug("SystematicsProcessor::processSystematics", key.str(), "matrix size", cov.GetNrows(), "x",
                       cov.GetNcols());
            result.covariance_matrices_.insert_or_assign(key, cov);
        }
        combineCovariances(result);
        log::debug("SystematicsProcessor::processSystematics", "Covariance calculation complete");
    }

    void clearFutures() { systematic_futures_.variations.clear(); }

    bool hasSystematics() const { return !systematic_futures_.variations.empty(); }

  private:
    static void sanitiseMatrix(TMatrixDSym &m) {
        const int rows = m.GetNrows();
        const int cols = m.GetNcols();
        tbb::parallel_for(tbb::blocked_range<int>(0, rows),
                          [&](const tbb::blocked_range<int> &r) {
            for (int i = r.begin(); i != r.end(); ++i) {
                const int max_col = std::min(i + 1, cols);
                for (int j = 0; j < max_col; ++j) {
                    double &val = m(i, j);
                    if (!std::isfinite(val)) val = 0.0;
                }
            }
        });
    }

    static void combineCovariances(VariableResult &result) {
        const int n_bins = result.total_mc_hist_.getNumberOfBins();
        if (n_bins <= 0) return;

        result.total_covariance_.ResizeTo(n_bins, n_bins);
        result.total_covariance_ = result.total_mc_hist_.hist.covariance();

        log::debug("SystematicsProcessor::combineCovariances", "Combining covariance matrices");
        for (const auto &[name, cov_matrix] : result.covariance_matrices_) {
            if (cov_matrix.GetNrows() == n_bins) {
                TMatrixDSym cov = cov_matrix;
                SystematicsProcessor::sanitiseMatrix(cov);
                log::debug("SystematicsProcessor::combineCovariances", "Adding matrix", name.str());
                result.total_covariance_ += cov;
            } else {
                log::warn("SystematicsProcessor::combineCovariances", "Skipping systematic", name.str(),
                          "due to incompatible matrix size (", cov_matrix.GetNrows(), "x", cov_matrix.GetNcols(),
                          "vs expected", n_bins, "x", n_bins, ")");
            }
        }

        SystematicsProcessor::sanitiseMatrix(result.total_covariance_);

        result.nominal_with_band_ = result.total_mc_hist_;
        result.nominal_with_band_.hist.shifts.resize(n_bins, 0);
        result.nominal_with_band_.addCovariance(result.total_covariance_);
    }

    static std::vector<KnobDef> createKnobs(const VariableRegistry &registry) {
        std::vector<KnobDef> out;
        out.reserve(registry.knobVariations().size());
        for (const auto &[name, cols] : registry.knobVariations()) {
            out.push_back({name, cols.first, cols.second});
        }
        return out;
    }

    static std::vector<UniverseDef> createUniverses(const VariableRegistry &registry) {
        std::vector<UniverseDef> out;
        out.reserve(registry.multiUniverseVariations().size());
        for (const auto &[name, count] : registry.multiUniverseVariations()) {
            out.push_back({name, name, count});
        }
        return out;
    }

    std::vector<std::unique_ptr<SystematicStrategy>> systematic_strategies_;
    std::vector<KnobDef> knob_definitions_;
    std::vector<UniverseDef> universe_definitions_;
    bool store_universe_hists_;
    SystematicFutures systematic_futures_;
};

}

#endif
