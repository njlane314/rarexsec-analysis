#ifndef SYSTEMATICS_PROCESSOR_H
#define SYSTEMATICS_PROCESSOR_H

#include <map>
#include <memory>
#include <vector>
#include <string>
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
        std::vector<UniverseDef>      universe_defs,
        std::vector<int>              channel_keys,
        BookHistFn                    book_fn
    )
      : channel_keys_(std::move(channel_keys))
      , book_fn_(std::move(book_fn))
      , knob_defs_(std::move(knob_defs))
      , universe_defs_(std::move(universe_defs))
    {
        for (auto& kd : knob_defs_) {
            strategies_.emplace_back(
                std::make_unique<WeightSystematicStrategy>(kd, book_fn_)
            );
        }
        for (auto& ud : universe_defs_) {
            strategies_.emplace_back(
                std::make_unique<UniverseSystematicStrategy>(ud, book_fn_)
            );
        }
        strategies_.emplace_back(
            std::make_unique<NormalisationSystematicStrategy>("norm", 0.03)
        );
    }

    void bookAll() {
        for (auto& strat : strategies_) {
            strat->bookVariations(channel_keys_, book_fn_);
        }
    }

    std::map<std::string, TMatrixDSym> computeAllCovariances(
        const BinnedHistogram& nominal_hist
    ) const {
        std::map<std::string, TMatrixDSym> out;
        for (auto& strat : strategies_) {
            for (auto key : channel_keys_) {
                out[strat->getName()] = strat->computeCovariance(key, nominal_hist);
            }
        }
        return out;
    }

    std::map<std::string, std::map<std::string, BinnedHistogram>>
    getAllVariedHistograms() const {
        std::map<std::string, std::map<std::string, BinnedHistogram>> out;
        for (auto& strat : strategies_) {
            for (auto key : channel_keys_) {
                out[strat->getName()] = strat->getVariedHistograms(key);
            }
        }
        return out;
    }

    const std::vector<KnobDef>& getKnobDefs() const { return knob_defs_; }
    const std::vector<UniverseDef>& getUniverseDefs() const { return universe_defs_; }
    void bookVariations(int channel_key) {
        for (auto& strat : strategies_) {
            strat->bookVariations({channel_key}, book_fn_);
        }
    }


private:
    std::vector<int>                                        channel_keys_;
    BookHistFn                                              book_fn_;
    std::vector<std::unique_ptr<SystematicStrategy>>        strategies_;
    std::vector<KnobDef>                                    knob_defs_;
    std::vector<UniverseDef>                                universe_defs_;
};

}

#endif