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
#include "NormalisationSystematicStrategy.h"
#include "DetectorSystematicStrategy.h"

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
        systematic_strategies_.emplace_back(
            std::make_unique<NormalisationSystematicStrategy>("normalisation", 0.03)
        );
    }

    void bookSystematics(
        const SampleKey& sample_key,
        const ROOT::RDF::RNode& rnode,
        const BinningDefinition& binning,
        const ROOT::RDF::TH1DModel& model
    ) {
        auto book_hist_fn = [&](const std::string& weight_column) {
            return rnode.Histo1D(model, binning.getVariable().Data(), weight_column);
        };
        for (const auto& strategy : systematic_strategies_) {
            strategy->bookVariations(sample_key, book_hist_fn, systematic_futures_);
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
            result.total_covariance_ = result.total_mc_hist_.cov; 
            for (const auto& [name, cov] : result.covariance_matrices_) {
                if (cov.GetNrows() == n_bins) {
                    result.total_covariance_ += cov;
                }
            }
            result.nominal_with_band_ = result.total_mc_hist_;
            result.nominal_with_band_.cov = result.total_covariance_;
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