#include <iostream>
#include "DataManager.h"
#include "EventDisplay.h"
#include "AnalysisSpace.h"
#include "Selection.h"

int main() {
    try {
        //ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager::Params params = {
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {}
        };
        AnalysisFramework::DataManager data_manager(params);

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 30, 0, 2)
            .defineRegion("numu_loose", "Loose NuMu Selection", "NUMU_CC_LOOSE", "QUALITY");

        int img_size = 512;
        std::string output_dir = "event_display_plots";
        AnalysisFramework::EventDisplay event_display(data_manager, img_size, output_dir);

        std::string selection_key = "SIGNAL";
        std::string preselection_key = "QUALITY";
        std::string additional_selection = "";
        int num_events = 5;
        std::string output_file = "event_display_numu_tight.pdf";

        /*event_display.VisualiseEventsInRegion(
            selection_key, preselection_key, additional_selection, num_events, output_file
        );*/

        event_display.visualiseSemanticViews(
            selection_key, preselection_key, additional_selection, num_events
        );

        std::cout << "Event displays generated successfully in " << output_dir << "/" << output_file << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}