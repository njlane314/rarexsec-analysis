#include <iostream>
#include <string>
#include <vector>

#include "framework/AnalysisFramework.h"

int main() {
    try {
        AnalysisFramework::DataManager::Params dm_params {
            .config_file = "config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {
                .load_reco_event_info = true,
                .load_truth_event_info = true,
                .load_weights_and_systematics = true
            }
        };
        AnalysisFramework::DataManager data_manager(dm_params);

        std::cout << "set analysis space" << std::endl;
        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 50, 0, 5)
            .defineRegion("numu_cc_sel", "NuMu CC Selection", "NUMU_CC", "QUALITY");

        std::cout << "set analysis space" << std::endl;
        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());
        systematics_controller
            .addWeightSystematic("RPA")
            .addWeightSystematic("CCMEC");
        
        std::cout << "set analysis space" << std::endl;
        AnalysisFramework::AnalysisRunner::Parameters runner_params {
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller
        };
        AnalysisFramework::AnalysisRunner runner(runner_params);
        
        std::cout << "set analysis space" << std::endl;
        auto results = runner.run();

        std::cout << "Analysis run completed successfully." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}