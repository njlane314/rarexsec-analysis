#include <iostream>
#include <string>
#include <vector>
#include <exception>

#include "ROOT/RDataFrame.hxx"
#include "AnalysisWorkflow.h"

int main() {
    try {
        ROOT::EnableImplicitMT();
        
        std::cout << "Initializing AnalysisWorkflow..." << std::endl;
        AnalysisFramework::AnalysisWorkflow workflow(
            "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json",
            "numi_fhc",
            {"run1"},
            true,
            "inclusive_strange_channels",
            "plots_snapshot"
        );

        std::vector<std::string> columns_to_save = {
            "run", "sub", "evt",
            "detector_image_u", "detector_image_v", "detector_image_w",
            "semantic_image_u", "semantic_image_v", "semantic_image_w",
            "inclusive_strange_channels",
            "exclusive_strange_channels",
            "central_value_weight"
        };
        
        std::cout << "Starting data snapshot..." << std::endl;
        workflow.snapshotDataFrames("NUMU_CC", "QUALITY", "numucc_snapshot.root", columns_to_save);
        std::cout << "Snapshot completed successfully and saved to numucc_snapshot.root!" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "An exception occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}