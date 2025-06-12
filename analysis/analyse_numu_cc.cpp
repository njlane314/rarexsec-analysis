#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "ROOT/RDataFrame.hxx"
#include "TString.h"
#include "TSystem.h"

#include "AnalysisFramework.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::AnalysisWorkflow workflow(
            "/exp/uboone/app/users/nlane/analysis/rarexsec_analysis/config.json",
            "numi_fhc",
            {"run1"},
            true,
            "analysis_channel",
            "plots"
        );

        workflow.loadAnalysisSpace("muon");

        std::cout << "Running analysis..." << std::endl;
        auto results = workflow.runAnalysis();
        std::cout << "Analysis run completed successfully!" << std::endl;

        for (const auto& pair : results) {
            const std::string& task_key = pair.first;
            const AnalysisFramework::AnalysisResult& result = pair.second;
            workflow.saveStackedPlot("stacked_" + task_key, result);
        }
        std::cout << "Plotting completed successfully! Plots are in the 'plots' directory." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}