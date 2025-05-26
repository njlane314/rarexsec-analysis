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
                .load_weights_and_systematics = true
            }
        });

        AnalysisFramework::DataSaver saver;
        saver.Save(dataframes_dict, {"run1"}, "NUMU_CC", "NUMU", "filtered_data.root", 
                                    {"run", "sub", "evt", 
                                    "event_weight", "event_category",
                                    "raw_image_u", "raw_image_v", "raw_image_w",
                                    "reco_image_u", "reco_image_v", "reco_image_w",
                                    "true_image_u", "true_image_v", "true_image_w"});

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}