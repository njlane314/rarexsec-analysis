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

#include "ROOT/RDataFrame.hxx"
#include "TH1.h"
#include "TMatrixDSym.h"

namespace AnalysisFramework {

class SystematicsController {
public:
    explicit SystematicsController(const VariableManager& var_manager) : var_manager_(var_manager) {}

    SystematicsController& addWeightSystematic(const std::string& name) {
        for (const auto& knob : var_manager_.GetKnobVariations()) {
            if (knob.first == name) {
                systematics_.push_back(std::make_unique<WeightSystematic>(knob.first, knob.second.first, knob.second.second));
                return *this;
            }
        }
        throw std::runtime_error("Weight systematic '" + name + "' not found in VariableManager.");
    }

    SystematicsController& addUniverseSystematic(const std::string& name) {
        const auto& univ_defs = var_manager_.GetMultiUniverseDefinitions();
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

    void bookVariations(const std::string& task_id, const std::string& sample_key, ROOT::RDF::RNode df, const DataManager::AssociatedVariationMap& det_var_nodes, const Binning& binning) {
        int category_id_placeholder = 0;
        std::string selection_query_placeholder = "";
        for (const auto& syst : systematics_) {
            syst->Book(df, det_var_nodes, sample_key, category_id_placeholder, binning, selection_query_placeholder);
        }
    }

    std::map<std::string, TMatrixDSym> computeAllCovariances(const std::string& task_id,
                                                             const Histogram& total_nominal_mc,
                                                             const Binning& binning_def) {
        std::map<std::string, TMatrixDSym> breakdown;
        int category_id_placeholder = 0;

        for (const auto& syst : systematics_) {
            breakdown[syst->GetName()] = syst->ComputeCovariance(category_id_placeholder, total_nominal_mc, binning_def);
        }
        return breakdown;
    }

    std::map<std::string, std::map<std::string, Histogram>> getAllVariedHistograms(const Binning& binning_def) {
        std::map<std::string, std::map<std::string, Histogram>> all_varied_hists;
        int category_id_placeholder = 0;
        for (const auto& syst : systematics_) {
            all_varied_hists[syst->GetName()] = syst->GetVariedHistograms(category_id_placeholder, binning_def);
        }
        return all_varied_hists;
    }


private:
    const VariableManager& var_manager_;
    std::vector<std::unique_ptr<Systematic>> systematics_;
};

}

#endif