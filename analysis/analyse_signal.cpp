#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "ROOT/RDataFrame.hxx"

#include "AnalysisFramework.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {}
        });

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("nu_e", "nu_e", "True Neutrino Energy", 60, 0.0, 12.0)
            .defineVariable("trk_len_v", "trk_len_v", "Track Length (cm)", 100, 0.0, 200.0, false, "", true)
            .defineRegion("ALL_EVENTS", "Empty Selection", {"ALL_EVENTS"})
            .defineRegion("SIGNAL", "Signal Filter", {"SIGNAL"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner_inclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "inclusive_strange_channels",
            .particle_category_column = "backtracked_pdg",     
            .particle_category_scheme = "particle_pdg_channels"
        });

        AnalysisFramework::AnalysisRunner runner_exclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "exclusive_strange_channels",
            .particle_category_column = "backtracked_pdg",     
            .particle_category_scheme = "particle_pdg_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_inclusive_phase_space = runner_inclusive.run();
        AnalysisFramework::AnalysisPhaseSpace analysis_exclusive_phase_space = runner_exclusive.run();

        AnalysisFramework::PlotManager plot_manager;

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nu_e", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "nu_e", "ALL_EVENTS", "exclusive_strange_channels", true);   
       
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nu_e", "SIGNAL", "inclusive_strange_channels", false); 
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "nu_e", "SIGNAL", "exclusive_strange_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "ALL_EVENTS", "particle_pdg_channels", false);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}