#include "DataLoader.h"
#include "RunEventDisplay.h"
#include "Selection.h" 
#include "TCanvas.h"
#include "TSystem.h"
#include "TString.h"  

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>      

#include "ROOT/RDataFrame.hxx"

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

        const std::string sample_key = "numi_fhc_overlay_intrinsic_strangeness_run1"; 
        if (dataframes_dict.find(sample_key) == dataframes_dict.end()) {
            throw std::runtime_error("Sample key not found: " + sample_key);
        }

        TString numu_preselection_query_str;
        auto preselection_categories_map = AnalysisFramework::Selection::getPreselectionCategories();
        auto numu_presel_it = preselection_categories_map.find("NUMU");

        if (numu_presel_it != preselection_categories_map.end()) {
            numu_preselection_query_str = numu_presel_it->second.query;
        } else {
            throw std::runtime_error("NUMU preselection category not found in Selection::getPreselectionCategories().");
        }

        if (numu_preselection_query_str.IsNull() || numu_preselection_query_str.IsWhitespace()) {
            throw std::runtime_error("NUMU preselection query string is empty or invalid.");
        }

        TString nc_event_category_filter_str = "event_category == 21";

        TString combined_filter_query = numu_preselection_query_str + " && " + nc_event_category_filter_str;

        std::cout << "Applying filter to sample '" << sample_key 
                  << "': " << combined_filter_query.Data() << std::endl;

        std::vector<ROOT::RDF::RNode>& original_rnodes_for_sample = dataframes_dict.at(sample_key).second;
        
        std::vector<ROOT::RDF::RNode> filtered_rnodes;
        filtered_rnodes.reserve(original_rnodes_for_sample.size());

        for (ROOT::RDF::RNode& node : original_rnodes_for_sample) {
            filtered_rnodes.push_back(node.Filter(combined_filter_query.Data()));
        }
        
        dataframes_dict.at(sample_key).second = filtered_rnodes;

        AnalysisFramework::RunEventDisplay event_plotter(dataframes_dict, 512, ".");

        event_plotter.VisualiseInput(sample_key);
   
    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}