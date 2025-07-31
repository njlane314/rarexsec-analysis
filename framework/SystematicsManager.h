#ifndef SYSTEMATICS_MANAGER_H
#define SYSTEMATICS_MANAGER_H

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

class SystematicsManager {
public:
    explicit SystematicsManager(const VariableManager& var_manager) : var_manager_(var_manager) {
        std::cout << "[SystematicsManager] Initialized." << std::endl;
    }

    SystematicsManager& addWeightSystematic(const std::string& name) {
        for (const auto& knob : var_manager_.getKnobVariations()) {
            if (knob.first == name) {
                weight_systematics_.push_back(std::make_unique<WeightSystematic>(knob.first, knob.second.first, knob.second.second));
                std::cout << "[SystematicsManager::addWeightSystematic] INFO: Added weight systematic '" << name << "'" << std::endl;
                return *this;
            }
        }
        throw std::runtime_error("[SystematicsManager::addWeightSystematic] FATAL: Weight systematic '" + name + "' not found in VariableManager.");
    }

    SystematicsManager& addUniverseSystematic(const std::string& name) {
        const auto& univ_defs = var_manager_.getMultiUniverseDefinitions();
        auto it = univ_defs.find(name);
        if (it != univ_defs.end()) {
            universe_systematics_.push_back(std::make_unique<UniverseSystematic>(name, it->first, it->second));
            std::cout << "[SystematicsManager::addUniverseSystematic] INFO: Added universe systematic '" << name << "' with " << it->second << " universes." << std::endl;
            return *this;
        }
        throw std::runtime_error("[SystematicsManager::addUniverseSystematic] FATAL: Universe systematic '" + name + "' not found in VariableManager.");
    }

    SystematicsManager& addDetectorSystematic(const std::string& name) {
        detector_systematics_.push_back(std::make_unique<DetectorVariationSystematic>(name));
        std::cout << "[SystematicsManager::addDetectorSystematic] INFO: Added detector systematic '" << name << "'" << std::endl;
        return *this;
    }

    SystematicsManager& addNormaliseUncertainty(const std::string& name, double uncertainty) {
        normalisation_systematics_.push_back(std::make_unique<NormalisationSystematic>(name, uncertainty));
        std::cout << "[SystematicsManager::addNormaliseUncertainty] INFO: Added normalisation uncertainty '" << name << "' with value " << uncertainty << std::endl;
        return *this;
    }

    void bookVariations(const std::string& task_id, const std::string& sample_key, ROOT::RDF::RNode df,
                        const DataManager::AssociatedVariationMap& det_var_nodes, const Binning& binning,
                        const std::string& selection_query, const std::string& category_column, const std::string& category_scheme) {
        std::cout << "[SystematicsManager::bookVariations] INFO: Booking variations for task '" << task_id << "', sample '" << sample_key << "'" << std::endl;
        auto analysis_channel_keys = getChannelKeys(category_scheme);
        for (int channel_key : analysis_channel_keys) {
            std::cout << "  |> Booking for channel key: " << channel_key << std::endl;
            for (const auto& syst : weight_systematics_) {
                syst->book(df, det_var_nodes, sample_key, channel_key, binning, selection_query, category_column, category_scheme);
            }
            for (const auto& syst : universe_systematics_) {
                syst->book(df, det_var_nodes, sample_key, channel_key, binning, selection_query, category_column, category_scheme);
            }
            for (const auto& syst : detector_systematics_) {
                syst->book(df, det_var_nodes, sample_key, channel_key, binning, selection_query, category_column, category_scheme);
            }
            for (const auto& syst : normalisation_systematics_) {
                syst->book(df, det_var_nodes, sample_key, channel_key, binning, selection_query, category_column, category_scheme);
            }
        }
    }

    std::map<std::string, TMatrixDSym> computeAllCovariances(int channel_key, const Histogram& nominal_hist,
                                                             const Binning& binning, const std::string& category_scheme) {
        std::cout << "[SystematicsManager::computeAllCovariances] INFO: Computing all covariances for channel key " << channel_key << std::endl;
        std::map<std::string, TMatrixDSym> breakdown;
        for (const auto& syst : weight_systematics_) {
            breakdown.emplace(syst->getName(), syst->computeCovariance(channel_key, nominal_hist, binning, category_scheme));
        }
        for (const auto& syst : universe_systematics_) {
            breakdown.emplace(syst->getName(), syst->computeCovariance(channel_key, nominal_hist, binning, category_scheme));
        }
        for (const auto& syst : detector_systematics_) {
            breakdown.emplace(syst->getName(), syst->computeCovariance(channel_key, nominal_hist, binning, category_scheme));
        }
        for (const auto& syst : normalisation_systematics_) {
            breakdown.emplace(syst->getName(), syst->computeCovariance(channel_key, nominal_hist, binning, category_scheme));
        }
        return breakdown;
    }

    std::map<std::string, std::map<std::string, Histogram>> getAllVariedHistograms(int channel_key,
                                                                                   const Binning& binning,
                                                                                   const std::string& category_scheme) {
        std::cout << "[SystematicsManager::getAllVariedHistograms] INFO: Retrieving all varied histograms for channel key " << channel_key << std::endl;
        std::map<std::string, std::map<std::string, Histogram>> all_varied_hists;
        for (const auto& syst : weight_systematics_) {
            all_varied_hists[syst->getName()] = syst->getVariedHistograms(channel_key, binning, category_scheme);
        }
        for (const auto& syst : universe_systematics_) {
            all_varied_hists[syst->getName()] = syst->getVariedHistograms(channel_key, binning, category_scheme);
        }
        for (const auto& syst : detector_systematics_) {
            all_varied_hists[syst->getName()] = syst->getVariedHistograms(channel_key, binning, category_scheme);
        }
        for (const auto& syst : normalisation_systematics_) {
            all_varied_hists[syst->getName()] = syst->getVariedHistograms(channel_key, binning, category_scheme);
        }
        return all_varied_hists;
    }

private:
    const VariableManager& var_manager_;
    std::vector<std::unique_ptr<WeightSystematic>> weight_systematics_;
    std::vector<std::unique_ptr<UniverseSystematic>> universe_systematics_;
    std::vector<std::unique_ptr<DetectorVariationSystematic>> detector_systematics_;
    std::vector<std::unique_ptr<NormalisationSystematic>> normalisation_systematics_;
};

}

#endif