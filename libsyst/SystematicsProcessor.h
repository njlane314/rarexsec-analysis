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
        std::vector<KnobDef>          knob_defs,
        std::vector<UniverseDef>      universe_defs
    )
      : knob_defs_(std::move(knob_defs))
      , universe_defs_(std::move(universe_defs))
    {
        for (auto& kd : knob_defs_) {
            strategies_.emplace_back(
                std::make_unique<WeightSystematicStrategy>(kd)
            );
        }
        for (auto& ud : universe_defs_) {
            strategies_.emplace_back(
                std::make_unique<UniverseSystematicStrategy>(ud)
            );
        }
        strategies_.emplace_back(
            std::make_unique<NormalisationSystematicStrategy>("norm", 0.03)
        );
    }

    void bookAll(const std::vector<int>& keys, BookHistFn book_fn, SystematicFutures& futures) {
        for (auto& strat : strategies_) {
            strat->bookVariations(keys, book_fn, futures);
        }
    }

    std::map<std::string, TMatrixDSym> computeCovariances(
        int key,
        const BinnedHistogram& nominal_hist,
        SystematicFutures& futures
    ) {
        std::map<std::string, TMatrixDSym> out;
        for (auto& strat : strategies_) {
            out[strat->getName()] = strat->computeCovariance(key, nominal_hist, futures);
        }
        return out;
    }

    std::map<std::string, std::map<std::string, BinnedHistogram>>
    getVariedHistograms(int key, const BinDefinition& bin, SystematicFutures& futures) {
        std::map<std::string, std::map<std::string, BinnedHistogram>> out;
        for (auto& strat : strategies_) {
             out[strat->getName()] = strat->getVariedHistograms(key, bin, futures);
        }
        return out;
    }

private:
    std::vector<std::unique_ptr<SystematicStrategy>>        strategies_;
    std::vector<KnobDef>                                    knob_defs_;
    std::vector<UniverseDef>                                universe_defs_;
};

}

#endif