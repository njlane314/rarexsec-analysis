#include "DataLoader.h"
#include "DataSaver.h"
#include <stdexcept>
#include <vector>
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
                .load_truth_event_info = true,
                .load_weights_and_systematics = false,
                .load_signal_weights = false
            }
        });

        AnalysisFramework::DataSaver saver;
        saver.Save(dataframes_dict, {"run1"}, "NUMU_CC", "NUMU", "filtered_data.root");

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}