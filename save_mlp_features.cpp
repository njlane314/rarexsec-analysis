#include <iostream>
#include <vector>
#include <string>

#include "AnalysisFramework.h"


int main() {
    AnalysisFramework::DataManager::Params dm_params;
    dm_params.config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json";
    dm_params.beam_key = "numi_fhc";
    dm_params.runs_to_load = {"run1"};
    dm_params.blinded = true;

    dm_params.variable_options.load_reco_event_info = true;
    dm_params.variable_options.load_reco_track_info = true;
    dm_params.variable_options.load_truth_event_info = false;
    dm_params.variable_options.load_weights_and_systematics = false;

    AnalysisFramework::DataManager data_manager(dm_params);

    std::string preselection_key = "QUALITY";
    std::string selection_key = "NUMU_CC_BACKGROUND"; 

    std::vector<std::string> columns_to_save = {
        "evnhits",
        "slnhits",
        "n_pfps",
        "n_tracks",
        "n_showers",
        "nu_slice_topo_score",
        "slclustfrac",
        "_opfilter_pe_beam",
        "run", "sub", "evt",
        "event_weight_cv",
        "analysis_channel",
        "n_muon_candidates",
        "selected_muon_length",
        "selected_muon_momentum_range",
        "selected_muon_momentum_mcs",
        "selected_muon_phi",
        "selected_muon_cos_theta",
        "selected_muon_energy",
        "selected_muon_trk_score",
        "selected_muon_llr_pid_score",
        "mcf_nmm", "mcf_nmp", "mcf_nem", "mcf_nep", "mcf_np0", "mcf_npp", "mcf_npm", "mcf_npr", "mcf_nne"
    };
    
    std::string output_file = "mlp_features_numucc_background_snapshot.root"; 

    std::cout << "Saving snapshot to " << output_file << "..." << std::endl;
    std::cout << "Applying Preselection: " << preselection_key << ", Selection: " << selection_key << std::endl;
    
    try {
        data_manager.Save(selection_key, preselection_key, output_file, columns_to_save);
        std::cout << "Snapshot successfully saved." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "An error occurred during snapshot saving: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
