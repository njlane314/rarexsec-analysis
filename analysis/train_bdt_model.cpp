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

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        std::cout << "Starting BDT training..." << std::endl;
        AnalysisFramework::BDTManager bdt_manager;

        std::vector<std::string> bdt_features = {
            "nhits_u", 
            "nhits_v",
            "nhits_w", 
            "nclusters_u",
            "nclusters_v",
            "nclusters_w"
        };

        std::string signal_cut = "(mcf_strangeness > 0)";
        std::string background_cut = "!(mcf_strangeness > 0)";

        std::string bdt_model_output_path = "MyBDT.root";
        std::string bdt_method_name = "BDT";
        std::string bdt_method_options = "!H:!V:NTrees=200:MinNodeSize=1%:MaxDepth=4:BoostType=AdaBoost:AdaBoostBeta=0.5:UseBaggedBoost:BaggedSampleFraction=0.5:SeparationType=GiniIndex:nCuts=20";

        TChain* mc_chain = new TChain("nuselection/EventSelectionFilter");

        bool has_mc_samples = false;
        double total_mc_pot = 0.0;

        AnalysisFramework::ConfigurationManager temp_config_mgr("/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json");
        const auto& run_config = temp_config_mgr.getRunConfig(data_manager.getBeamKey(), data_manager.getRunsToLoad()[0]);

        for (const auto& sample_prop_pair : run_config.sample_props) {
            const auto& sample_props = sample_prop_pair.second;
            if (sample_props.sample_type == AnalysisFramework::SampleType::kMonteCarlo) {
                std::string full_path = temp_config_mgr.getBaseDirectory() + "/" + sample_props.relative_path;
                mc_chain->Add(full_path.c_str());
                total_mc_pot += sample_props.pot;
                has_mc_samples = true;
            }
        }

        if (has_mc_samples) {
            ROOT::RDF::RNode training_df_base = ROOT::RDataFrame(*mc_chain);

            double data_pot_for_scaling = data_manager.getDataPOT();
            double calculated_event_weight_for_chain = 1.0;
            if (total_mc_pot > 0 && data_pot_for_scaling > 0) {
                calculated_event_weight_for_chain = data_pot_for_scaling / total_mc_pot;
            }

            ROOT::RDF::RNode training_df_with_base_weight = training_df_base.Define("base_event_weight", [calculated_event_weight_for_chain](){ return calculated_event_weight_for_chain; });

            AnalysisFramework::DefinitionManager definition_manager(data_manager.getVariableManager());

            ROOT::RDF::RNode defined_training_df = definition_manager.processNode(
                training_df_with_base_weight,
                AnalysisFramework::SampleType::kMonteCarlo,
                data_manager.getVariableOptions(),
                false
            );
            
            std::string quality_presel_query = AnalysisFramework::Selection::getPreselectionCategories().at("QUALITY").query.Data();
            ROOT::RDF::RNode filtered_training_df = defined_training_df.Filter(quality_presel_query);

            bdt_manager.trainBDT(
                filtered_training_df,
                bdt_features,
                signal_cut,
                background_cut,
                bdt_model_output_path,
                bdt_method_name,
                bdt_method_options
            );
            std::cout << "BDT training complete. Model saved to: " << bdt_model_output_path << std::endl;
        } else {
            std::cerr << "Warning: No Monte Carlo samples found for BDT training. Skipping BDT training." << std::endl;
        }

        delete mc_chain;

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}