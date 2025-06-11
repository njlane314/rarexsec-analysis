#include <iostream>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "AnalysisFramework.h"

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
            .defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 100, 0, 2)
            .defineVariable("neutrino_energy", "nu_e", "Neutrino Energy [GeV]", 100, 0, 10)
            .defineVariable("slice_hits", "slnhits", "Slice Hits", 80, 0, 8000)
            .defineRegion("base_sel", "Base Selection", "", "QUALITY")
            .defineRegion("numu_cc_sel", "NuMu CC Selection", "NUMU_CC", "QUALITY")
            .defineRegion("signal", "Signal Selection", "SIGNAL", "QUALITY")
            .defineRegion("nc", "Neutral Current Filter", "NC", "QUALITY");

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .category_column = "analysis_channel"
        });
        auto results = runner.run();

        AnalysisFramework::PlotManager plot_manager("plots");
        plot_manager.saveStackedPlot("muon_momentum_stacked", results.at("muon_momentum@numu_cc_sel"));
        plot_manager.saveStackedPlot("neutrino_energy_base_stacked", results.at("neutrino_energy@base_sel"));
        plot_manager.saveStackedPlot("neutrino_energy_numucc_stacked", results.at("neutrino_energy@numu_cc_sel"));
        plot_manager.saveStackedPlot("slice_hits_signal_stacked", results.at("slice_hits@signal"));
        plot_manager.saveStackedPlot("slice_hits_nc_stacked", results.at("slice_hits@nc"));

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}