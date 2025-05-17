#include "DatasetLoader.h"
#include "ConfigurationManager.h"
#include "VariableManager.h"

#include <iostream>
#include <stdexcept>

int main() {
    try {
        AnalysisFramework::FilePathConfiguration file_paths;
        file_paths.sample_directory_base = "/exp/uboone/data/users/nlane/analysis/";
        AnalysisFramework::ConfigurationManager config_manager(file_paths);
        AnalysisFramework::VariableManager variable_manager;
        AnalysisFramework::DatasetLoader loader(config_manager, variable_manager);
        
        std::string beam_key = "numi_fhc";              
        std::vector<std::string> runs_to_load = {"run1"}; 
        bool blinded = true;                            
        AnalysisFramework::VariableOptions var_opts;      

        auto campaign = loader.LoadRuns(beam_key, runs_to_load, blinded, var_opts);
        long long total_events = 0;
        for (auto& [sample_key, dfs] : campaign.dataframes) {
            for (auto& df : dfs) {
                auto count = df.Count().GetValue();
                total_events += count;
                if (df.HasColumn("exposure_event_weight") && count > 0) {
                    auto weight = df.Min<double>("exposure_event_weight").GetValue();
                    std::cout << "Sample: " << sample_key << ", pot_scale_weight: " << weight << std::endl;
                }
            }
        }         
                
        std::cout << "Total number of events in the campaign: " << total_events << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}