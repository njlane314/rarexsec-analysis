#ifndef SYSTEMATICS_PROCESSOR_H
#define SYSTEMATICS_PROCESSOR_H

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"
#include "WeightSystematicStrategy.h"
#include "UniverseSystematicStrategy.h"
#include "DetectorSystematicStrategy.h"
#include "Logger.h"

namespace analysis {

class SystematicsProcessor {
public:
    SystematicsProcessor(
        std::vector<KnobDef> knob_definitions,
        std::vector<UniverseDef> universe_definitions
    )
      : knob_definitions_(std::move(knob_definitions))
      , universe_definitions_(std::move(universe_definitions))
    {
        systematic_strategies_.emplace_back(
            std::make_unique<DetectorSystematicStrategy>()
        );
        for (const auto& knob : knob_definitions_) {
            systematic_strategies_.emplace_back(
                std::make_unique<WeightSystematicStrategy>(knob)
            );
        }
        for (const auto& universe : universe_definitions_) {
            systematic_strategies_.emplace_back(
                std::make_unique<UniverseSystematicStrategy>(universe)
            );
        }
    }

    void bookSystematics(
        const SampleKey& sample_key,
        ROOT::RDF::RNode& rnode,
        const BinningDefinition& binning,
        const ROOT::RDF::TH1DModel& model
    ) {
        for (const auto& strategy : systematic_strategies_) {
            strategy->bookVariations(sample_key, rnode, binning, model, systematic_futures_);
        }
    }

    void processSystematics(VariableResult& result) {
        for (const auto& strategy : systematic_strategies_) {
            SystematicKey key{strategy->getName()};
            result.covariance_matrices_[key] = strategy->computeCovariance(
                result, systematic_futures_
            );
        }

        int n_bins = result.total_mc_hist_.getNumberOfBins();
        if (n_bins > 0) {
            result.total_covariance_.ResizeTo(n_bins, n_bins);
            result.total_covariance_ = result.total_mc_hist_.covariance();
            for (const auto& [name, cov] : result.covariance_matrices_) {
                if (cov.GetNrows() == n_bins) {
                    result.total_covariance_ += cov;
                } else {
                    log::warn("SystematicsProcessor::processSystematics", "Skipping systematic", name.str(), "due to incompatible matrix size (", cov.GetNrows(), "x", cov.GetNcols(), "vs expected", n_bins, "x", n_bins, ")");
                }
            }
            result.nominal_with_band_ = result.total_mc_hist_;
            result.nominal_with_band_.shifts.resize(n_bins, 0);
            result.nominal_with_band_.addCovariance(result.total_covariance_);
        }
    }

    

private:
    std::vector<std::unique_ptr<SystematicStrategy>> systematic_strategies_;
    std::vector<KnobDef> knob_definitions_;
    std::vector<UniverseDef> universe_definitions_;
    SystematicFutures systematic_futures_;
};

}

#endif