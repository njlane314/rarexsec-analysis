#ifndef DEFINITIONMANAGER_H
#define DEFINITIONMANAGER_H

#include "ROOT/RDataFrame.hxx"
#include "VariableManager.h"
#include "DataTypes.h"
#include "ProcessingStep.h"
#include "TruthProcessor.h"
#include "EventProcessor.h"
#include <memory>
#include <vector>

namespace AnalysisFramework {

class DefinitionManager {
public:
    DefinitionManager(const VariableManager& var_mgr) : variable_manager_(var_mgr) {}

    ROOT::RDF::RNode processNode(ROOT::RDF::RNode df, SampleType sample_type) const {
        std::vector<std::unique_ptr<ProcessingStep>> processors;
        processors.push_back(std::make_unique<EventProcessor>());
        processors.push_back(std::make_unique<TruthProcessor>(sample_type));

        for(size_t i = 0; i < processors.size() - 1; ++i) {
            processors[i]->setNext(std::move(processors[i+1]));
        }

        auto processed_df = processors.front()->apply(df);
        
        if (sample_type == SampleType::kMonteCarlo) {
             processed_df = defineNominalWeight(processed_df);
             processed_df = defineKnobVariationWeights(processed_df);
        } else {
            if (!processed_df.HasColumn("central_value_weight") && processed_df.HasColumn("base_event_weight")) {
                processed_df = processed_df.Alias("central_value_weight", "base_event_weight");
            } else if (!processed_df.HasColumn("central_value_weight")) {
                processed_df = processed_df.Define("central_value_weight", "1.0");
            }
        }
        
        return processed_df;
    }

private:
    const VariableManager& variable_manager_;

    ROOT::RDF::RNode defineNominalWeight(ROOT::RDF::RNode df) const {
        auto standard_weight_calculator = [](double w, float w_spline, float w_tune) {
            double final_weight = w;
            if (std::isfinite(w_spline) && w_spline > 0) final_weight *= w_spline;
            if (std::isfinite(w_tune) && w_tune > 0) final_weight *= w_tune;
            if (!std::isfinite(final_weight) || final_weight < 0) return 1.0;
            return final_weight;
        };
        return df.Define("central_value_weight", standard_weight_calculator, {"base_event_weight", "weightSpline", "weightTune"});
    }

    ROOT::RDF::RNode defineKnobVariationWeights(ROOT::RDF::RNode df) const {
        if (!df.HasColumn("central_value_weight")) return df;
        
        auto knob_variations = variable_manager_.getKnobVariations();
        for (const auto& knob : knob_variations) {
            if (df.HasColumn(knob.second.first)) {
                df = df.Define("weight_" + knob.first + "_up", "central_value_weight * " + knob.second.first);
            }
            if (df.HasColumn(knob.second.second)) {
                df = df.Define("weight_" + knob.first + "_dn", "central_value_weight * " + knob.second.second);
            }
        }
        return df;
    }
};

}
#endif