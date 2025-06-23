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
            .defineVariable("n_blips", "n_blips", "Number of Blips", 20, 0, 20)
            .defineVariable("total_blip_energy", "total_blip_energy", "Total Blip Energy [MeV]", 100, 0.0, 500.0)
            .defineRegion("NUMU_CC", "NuMu CC Selection", {"QUALITY", "NUMU_CC"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner_inclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "inclusive_strange_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_inclusive_phase_space = runner_inclusive.run();

        AnalysisFramework::PlotManager plot_manager;

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_blips", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_blip_energy", "NUMU_CC", "inclusive_strange_channels");
        
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}