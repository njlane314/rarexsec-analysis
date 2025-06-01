#include <iostream>
#include <string>
#include <vector>
#include <exception>
#include <memory>

#include "ROOT/RDataFrame.hxx"

#include "framework/DataManager.h"
#include "framework/CorrelationManager.h"
#include "framework/VariableManager.h"
#include "framework/DataTypes.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager::Params dm_params {
            .config_file = "../config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {
                .load_reco_event_info = true,
                .load_reco_track_info = true,
                .load_truth_event_info = true,
                .load_weights_and_systematics = true,
                .load_reco_shower_info = false,
                .load_blip_info = false
            }
        };
        AnalysisFramework::DataManager data_manager(dm_params);

        std::string target_mc_sample_key = "mc_strangeness_run1_fhc";
        
        const auto& all_samples_map = data_manager.getAllSamples();
        auto sample_it = all_samples_map.find(target_mc_sample_key);

        if (sample_it == all_samples_map.end() || !sample_it->second.isMonteCarlo()) {
            throw std::runtime_error("Target MC sample '" + target_mc_sample_key + "' not found or not MC type in DataManager.");
        }
        
        ROOT::RDF::RNode analysis_df = sample_it->second.getDataFrame();
        
        std::vector<std::string> final_state_particles = {
            "mcf_nmm", "mcf_nmp", "mcf_nem", "mcf_nep", "mcf_np0", "mcf_npp", "mcf_npm", "mcf_npr",
            "mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m"
        };
        
        // Correctly get both up and down knobs from the VariableManager
        AnalysisFramework::VariableManager var_manager; // Create an instance
        std::vector<std::string> genie_knobs;
        for (const auto& knob_pair : var_manager.GetKnobVariations()) {
            genie_knobs.push_back(knob_pair.second.first);  // Add the "up" knob
            genie_knobs.push_back(knob_pair.second.second); // Add the "down" knob
        }
        
        std::vector<std::string> all_vars;
        all_vars.insert(all_vars.end(), final_state_particles.begin(), final_state_particles.end());
        all_vars.insert(all_vars.end(), genie_knobs.begin(), genie_knobs.end());

        AnalysisFramework::CorrelationManager correlation_manager(analysis_df, 
                                                                "SIGNAL",      
                                                                "QUALITY",     
                                                                all_vars);
        
        std::cout << "Running correlation analysis..." << std::endl;
        correlation_manager.Run(); 
        
        std::cout << "Plotting correlation matrix..." << std::endl;
        correlation_manager.Plot("plots/correlation_matrix_signal.png");
        std::cout << "Plot saved to plots/correlation_matrix_signal.png" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}