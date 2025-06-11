#include <iostream>
#include <string>
#include <vector>
#include <exception>

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
            .variable_options = {}
        });

        std::vector<std::string> columns_to_save = {
            "run", "sub", "evt",
            "raw_image_u", "raw_image_v", "raw_image_w",
            "true_image_u", "true_image_v", "true_image_w",
            "analysis_channel", "event_weight_cv"
        };

        data_manager.snapshotDataFrames("NUMU_CC", "QUALITY", "numucc_snapshot.root", columns_to_save);
        std::cout << "Snapshot of filtered events saved!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}