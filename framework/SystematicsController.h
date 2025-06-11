#ifndef SYSTEMATICS_CONTROLLER_H
#define SYSTEMATICS_CONTROLLER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <iostream>

#include "Systematics.h"
#include "VariableManager.h"
#include "Histogram.h"
#include "DataManager.h"
#include "AnalysisChannels.h"

#include "ROOT/RDataFrame.hxx"
#include "TH1.h"
#include "TMatrixDSym.h"

namespace AnalysisFramework {

class SystematicsController {
public:
    explicit SystematicsController(const VariableManager& var_manager) : var_manager_(var_manager) {}

    SystematicsController& addWeightSystematic(const std::string& name) {
        for (const auto& knob : var_manager_.getKnobVariations()) {
            if (knob.first == name) {
                systematics_.push_back(std::make_unique<WeightSystematic>(knob.first, knob.second.first, knob.second.second));
                return *this;
            }
        }
        throw std::runtime_error("Weight systematic '" + name + "' not found in VariableManager.");
    }

    SystematicsController& addUniverseSystematic(const std::string& name) {
        const auto& univ_defs = var_manager_.getMultiUniverseDefinitions();
        auto it = univ_defs.find(name);
        if (it != univ_defs.end()) {
            systematics_.push_back(std::make_unique<UniverseSystematic>(name, it->first, it->second));
            return *this;
        }
        throw std::runtime_error("Universe systematic '" + name + "' not found in VariableManager.");
    }

    SystematicsController& addDetectorSystematic(const std::string& name) {
        systematics_.push_back(std::make_unique<DetectorVariationSystematic>(name));
        return *this;
    }

    SystematicsController& addNormaliseUncertainty(const std::string& name, double uncertainty) {
        systematics_.push_back(std::make_unique<NormalisationSystematic>(name, uncertainty));
        return *this;
    }

    void bookVariations(const std::string& task_id, const std::string& sample_key, ROOT::RDF::RNode df,
                        const DataManager::AssociatedVariationMap& det_var_nodes, const Binning& binning,
                        const std::string& selection_query, const std::string& analysis_channel_column) {
        auto analysis_channel_keys = getChannelKeys(analysis_channel_column);
        for (int channel_key : analysis_channel_keys) {
            for (const auto& syst : systematics_) {
                syst->book(df, det_var_nodes, sample_key, channel_key, binning, selection_query, analysis_channel_column);
            }
        }
    }

    std::map<std::string, TMatrixDSym> computeAllCovariances(int channel_key, const Histogram& nominal_hist,
                                                             const Binning& binning, const std::string& analysis_channel_column) {
        std::map<std::string, TMatrixDSym> breakdown;
        for (const auto& syst : systematics_) {
            breakdown.emplace(syst->getName(), syst->computeCovariance(channel_key, nominal_hist, binning, analysis_channel_column));
        }
        return breakdown;
    }

    std::map<std::string, std::map<std::string, Histogram>> getAllVariedHistograms(int channel_key,
                                                                                   const Binning& binning,
                                                                                   const std::string& analysis_channel_column) {
        std::map<std::string, std::map<std::string, Histogram>> all_varied_hists;
        for (const auto& syst : systematics_) {
            all_varied_hists[syst->getName()] = syst->getVariedHistograms(channel_key, binning, analysis_channel_column);
        }
        return all_varied_hists;
    }

private:
    const VariableManager& var_manager_;
    std::vector<std::unique_ptr<Systematic>> systematics_;
};

}

#endif