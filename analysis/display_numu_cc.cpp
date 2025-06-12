#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "ROOT/RDataFrame.hxx"

#include "AnalysisFramework.h"

int main() {
    try {
        //ROOT::EnableImplicitMT();

        AnalysisFramework::AnalysisWorkflow workflow(
            "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json",
            "numi_fhc",
            {"run1"},
            true,
            "inclusive_strange_channels",
            "plots"
        );

        workflow.defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 30, 0, 2);
        workflow.defineRegion("numu_loose", "Loose NuMu Selection", "NUMU_CC_LOOSE", "QUALITY");

        std::string selection_key = "SIGNAL";
        std::string preselection_key = "QUALITY";
        std::string additional_selection = "";
        int num_events = 5;
        int img_size = 512;
        std::string output_dir = "event_display_plots";

        workflow.visualiseSemanticViews(
            selection_key, preselection_key, additional_selection, num_events, img_size, output_dir
        );

        std::cout << "Event displays generated successfully in " << output_dir << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}