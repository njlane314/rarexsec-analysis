#ifndef SYSTEMATICS_PROCESSOR_H
#define SYSTEMATICS_PROCESSOR_H

#include <map>
#include <memory>
#include <vector>
#include <string>
#include <TMatrixDSym.h>
#include "SystematicStrategy.h"
#include "BinnedHistogram.h"

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
    {
        for (auto& kd : knob_defs) {
            strategies_.emplace_back(
                std::make_unique<WeightSystematicStrategy>(kd, book_fn_)
            );
        }
        for (auto& ud : universe_defs) {
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
        const libhist/BinnedHistogram& nominal_hist
    ) const {
        std::map<std::string, TMatrixDSym> out;
        for (auto& strat : strategies_) {
            for (auto key : channel_keys_) {
                out[strat->getName()] = strat->computeCovariance(key, nominal_hist);
            }
        }
        return out;
    }

    std::map<std::string, std::map<std::string, libhist/BinnedHistogram>>
    getAllVariedHistograms() const {
        std::map<std::string, std::map<std::string, libhist/BinnedHistogram>> out;
        for (auto& strat : strategies_) {
            for (auto key : channel_keys_) {
                out[strat->getName()] = strat->getVariedHistograms(key);
            }
        }
        return out;
    }

private:
    std::vector<int>                                        channel_keys_;
    BookHistFn                                              book_fn_;
    std::vector<std::unique_ptr<SystematicStrategy>>        strategies_;
};

}

#endif // SYSTEMATICS_PROCESSOR_H