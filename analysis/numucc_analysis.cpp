#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "ROOT/RDataFrame.hxx"
#include "AnalysisFramework.h"
#include "TString.h"
#include "TSystem.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {
                .load_reco_event_info = true,
                .load_reco_track_info = true,
                .load_truth_event_info = true,
                .load_weights_and_systematics = true,
                .load_reco_shower_info = true,
                .load_blip_info = true
            }
        });

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 30, 0, 2)
            .defineVariable("muon_length", "selected_muon_length", "Muon Length [cm]", 50, 0, 500)
            .defineVariable("muon_cos_theta", "selected_muon_cos_theta", "Muon cos(#theta)", 40, -1, 1)
            .defineRegion("numu_loose", "Loose NuMu Selection", "NUMU_CC_LOOSE", "QUALITY")
            .defineRegion("numu_tight", "Tight NuMu Selection", "NUMU_CC_TIGHT", "QUALITY")
            .defineRegion("track_score", "Track Score Selection", "TRACK_SCORE", "QUALITY")
            .defineRegion("pid_score", "PID Score Selection", "PID_SCORE", "QUALITY")
            .defineRegion("fiducial", "Fiducial Volume Selection", "FIDUCIAL_VOLUME", "QUALITY")
            .defineRegion("track_length", "Track Length Selection", "TRACK_LENGTH", "QUALITY");


        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .category_column = "analysis_channel"
        });

        auto results = runner.run();
        std::cout << "Analysis run completed successfully." << std::endl;

        AnalysisFramework::PlotManager plot_manager("plots");

        std::cout << "Plotting completed successfully. Plots are in the 'plots' directory." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}