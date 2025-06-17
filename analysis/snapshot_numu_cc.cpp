#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <numeric>

#include "ROOT/RDataFrame.hxx"
#include "TChain.h"

#include "AnalysisFramework.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = false,
            .variable_options = {} 
        });

        std::string output_snapshot_file = "filtered_numu_cc_events.root";
        std::cout << "Target snapshot file: " << output_snapshot_file << std::endl;

        std::vector<std::string> columns_to_save = {
            "run", "sub", "evt",
            "base_event_weight",
            "inclusive_strange_channels",
            "exclusive_strange_channels",
            "detector_image_u", "detector_image_v", "detector_image_w",
            "semantic_image_u", "semantic_image_v", "semantic_image_w"
        };

        std::cout << "Snapshotting events after muon-neutrino charged current selection..." << std::endl;
        data_manager.snapshotDataFrames(
            "NUMU_CC",
            "QUALITY",
            output_snapshot_file,
            columns_to_save
        );

        std::cout << "Snapshot complete. Filtered events saved to: " << output_snapshot_file << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}