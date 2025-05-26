#include "DataLoader.h"
#include "RunEventDisplay.h"
#include "TCanvas.h"
#include "TSystem.h"

#include <iostream>
#include <stdexcept>
#include <string>

int main() {
    try {
        AnalysisFramework::DataLoader loader;
        auto [dataframes_dict, data_pot] = loader.LoadRuns({
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = AnalysisFramework::VariableOptions{
                .load_reco_event_info = true,
                .load_truth_event_info = false,
                .load_weights_and_systematics = false
            }
        });

        const std::string sample_key = "numi_fhc_overlay_intrinsic_strangeness_run1";
        if (dataframes_dict.find(sample_key) == dataframes_dict.end()) {
            throw std::runtime_error("Sample key not found: " + sample_key);
        }

        AnalysisFramework::RunEventDisplay event_plotter(dataframes_dict, 512, ".");

        event_plotter.VisualiseInput(sample_key);
   
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}