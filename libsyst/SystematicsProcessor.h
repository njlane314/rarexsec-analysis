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

    void bookAllSystematics(const std::vector<int>& stratification_keys,
                            const BookHistFn& histogram_booking_function,
                            SystematicFutures& systematic_futures) {
        for (const auto& strategy : systematic_strategies_) {
            strategy->bookVariations(stratification_keys, histogram_booking_function, systematic_futures);
        }
    }

    std::map<std::string, TMatrixDSym> computeCovarianceMatrices(
        int stratification_key,
        const BinnedHistogram& nominal_histogram,
        SystematicFutures& systematic_futures
    ) {
        std::map<std::string, TMatrixDSym> covariance_matrices;
        for (const auto& strategy : systematic_strategies_) {
            covariance_matrices[strategy->getName()] = strategy->computeCovariance(
                stratification_key, nominal_histogram, systematic_futures
            );
        }
        return covariance_matrices;
    }

    std::map<std::string, std::map<std::string, BinnedHistogram>>
    extractVariationHistograms(int stratification_key,
                               const BinDefinition& binning,
                               SystematicFutures& systematic_futures) {
        std::map<std::string, std::map<std::string, BinnedHistogram>> variation_histograms;
        for (const auto& strategy : systematic_strategies_) {
            variation_histograms[strategy->getName()] = strategy->getVariedHistograms(
                stratification_key, binning, systematic_futures
            );
        }
        return variation_histograms;
    }

private:
    std::vector<std::unique_ptr<SystematicStrategy>> systematic_strategies_;
    std::vector<KnobDef> knob_definitions_;
    std::vector<UniverseDef> universe_definitions_;
};

}

#endif