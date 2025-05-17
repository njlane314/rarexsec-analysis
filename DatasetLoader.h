#ifndef DATALOADER_H
#define DATALOADER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <optional>

#include "ROOT/RDataFrame.hxx"
#include "TString.h"
#include "TChain.h"

#include "SampleTypes.h"
#include "VariableManager.h"
#include "ConfigurationManager.h"
#include "DatasetProcessing.h"

namespace AnalysisFramework {

struct CampaignDataset {
    std::map<std::string, std::vector<ROOT::RDF::RNode>> dataframes;
    std::map<std::string, double> sample_scalars;
    double total_pot = 0.0;
    int total_triggers = 0;
};

struct ProcessingConfig {
    const RunConfiguration& run_config;
    VariableOptions variable_options;
    bool blinded;
};

struct SamplePath {
    const SampleProperties* sample_props; 
    std::string file_path;                                       
};

class DatasetLoader {
private:
    const ConfigurationManager& config_manager_;
    const VariableManager& variable_manager_;

    std::optional<SamplePath> ResolveSamplePath(
        const std::string& beam_key,          
        const std::string& run_key,      
        const std::string& sample_key) const {

        const auto& file_paths_config = config_manager_.GetFilePaths();
        SamplePath path_info;

        try {
            const auto& run_config = config_manager_.GetRunConfig(beam_key, run_key);
            path_info.sample_props = &(run_config.sample_props.at(sample_key));

            if (path_info.sample_props->relative_path.empty()) {
                throw std::runtime_error("DatasetLoader::ResolveSamplePath: Dataset '" + sample_key + "' in ProcessingConfig '" + run_key + 
                                         "' (beam: " + beam_key + ") has an empty relative_path.");
            }
            path_info.file_path = file_paths_config.sample_directory_base + "/" + path_info.sample_props->relative_path;
        } catch (const std::out_of_range& oor) {
            throw std::runtime_error("DatasetLoader::ResolveSamplePath: Configuration error for beam '" + beam_key +
                                     "', run_key '" + run_key + "', sample_key '" + sample_key +
                                     ": " + oor.what());
        }

        if (!std::ifstream(path_info.file_path.c_str()).good()) {
            throw std::runtime_error("DatasetLoader::ResolveSamplePath: File not found: " + path_info.file_path);
        }
        return path_info;
    }

    ROOT::RDF::RNode CreateDataFrameNode(
        const SamplePath& path_info,
        const VariableOptions& variable_options) const {
        std::vector<std::string> cols = variable_manager_.GetVariables(variable_options, path_info.sample_props->type);
        return ROOT::RDataFrame("nuselection/EventSelectionFilter", path_info.file_path, cols);
    }

    ROOT::RDF::RNode ApplyEventProcessing(
        ROOT::RDF::RNode df,
        const SamplePath& path_info,
        const VariableOptions& variable_options,
        bool is_detvar_sample) const {
        df = AddEventCategories(df, path_info.sample_props->type);
        df = ProcessNuMuVariables(df, path_info.sample_props->type);
        return df;
    }

    std::string BuildExclusiveFilter(const std::vector<std::string>& mc_keys, 
                                     const std::map<std::string, SampleProperties>& samples) const {
        std::string filter = "true";
        for (const auto& key : mc_keys) {
            if (samples.count(key) && !samples.at(key).truth_filter.empty()) {
                if (filter == "true") {
                    filter = "!(" + samples.at(key).truth_filter + ")";
                } else {
                    filter += " && !(" + samples.at(key).truth_filter + ")";
                }
            }
        }
        return filter;
    }

    void LoadSamples(
        CampaignDataset& campaign_dataset,
        const ProcessingConfig& processing_config) const
    {
        auto data_props_iter = std::find_if(
            processing_config.run_config.sample_props.begin(),
            processing_config.run_config.sample_props.end(),
            [](const auto& pair) { return isSampleData(pair.second.type); }
        );

        if (data_props_iter == processing_config.run_config.sample_props.end()) {
            throw std::runtime_error("Dataset::LoadSamples: No data sample found for run " + processing_config.run_config.run_key);
        }

        const auto& run_props = data_props_iter->second;
        double data_pot = run_props.pot;
        int run_triggers = run_props.triggers;

        campaign_dataset.total_pot += data_pot;
        campaign_dataset.total_triggers += run_triggers;

        // --- Load Data Samples -- 

        if (!processing_config.blinded) {
            for (const auto& sample_pair : processing_config.run_config.sample_props) {
                if (isSampleData(sample_pair.second.type) && !sample_pair.first.empty()) {
                    const std::string& data_key = sample_pair.first;
                    auto path_opt = this->ResolveSamplePath(processing_config.run_config.beam_key, processing_config.run_config.run_key, data_key);
                    if (path_opt) {
                        ROOT::RDF::RNode rdf_data = this->CreateDataFrameNode(path_opt.value(), processing_config.variable_options);
                        rdf_data = this->ApplyEventProcessing(rdf_data, path_opt.value(), processing_config.variable_options, false);
                        rdf_data = rdf_data.Define("exposure_event_weight", [](){ return 1.0; });
                        campaign_dataset.dataframes[data_key].push_back(rdf_data);
                    }
                }
            }
        }

        // --- Load External Samples --- 

        for (const auto& sample_pair : processing_config.run_config.sample_props) {
            if (isSampleEXT(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& ext_key = sample_pair.first;
                const auto& ext_props = sample_pair.second;
                auto path_opt = this->ResolveSamplePath(processing_config.run_config.beam_key, processing_config.run_config.run_key, ext_key);
                if (path_opt) {
                    ROOT::RDF::RNode rdf_ext = this->CreateDataFrameNode(path_opt.value(), processing_config.variable_options);
                    rdf_ext = this->ApplyEventProcessing(rdf_ext, path_opt.value(), processing_config.variable_options, false);
                    double ext_weight = 1.0;
                    if (ext_props.triggers > 0 && run_triggers > 0) {
                        ext_weight = static_cast<double>(run_triggers) / ext_props.triggers;
                    }
                    rdf_ext = rdf_ext.Define("exposure_event_weight", [ext_weight](){ return ext_weight; });
                    campaign_dataset.dataframes[ext_key].push_back(rdf_ext);
                }
            }
        }

        // --- Load Monte Carlo Samples ---

        for (const auto& sample_pair : processing_config.run_config.sample_props) {
            if (isSampleMC(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& mc_key = sample_pair.first;
                const auto& mc_props = sample_pair.second;
                auto path_opt = this->ResolveSamplePath(processing_config.run_config.beam_key, processing_config.run_config.run_key, mc_key);
                if (path_opt) {
                    ROOT::RDF::RNode rdf_mc = this->CreateDataFrameNode(path_opt.value(), processing_config.variable_options);
                    rdf_mc = this->ApplyEventProcessing(rdf_mc, path_opt.value(), processing_config.variable_options, false); 
                    if (!mc_props.truth_filter.empty()) {
                        rdf_mc = rdf_mc.Filter(mc_props.truth_filter);
                    }
                    if (!mc_props.exclusion_truth_filters.empty()) {
                        std::string exclusion_filter = this->BuildExclusiveFilter(
                            mc_props.exclusion_truth_filters,
                            processing_config.run_config.sample_props
                        );
                        if (exclusion_filter != "true") {
                            rdf_mc = rdf_mc.Filter(exclusion_filter);
                        }
                    }
                    double pot_weight = (mc_props.pot > 0 && data_pot > 0) ? (data_pot / mc_props.pot) : 1.0;
                    rdf_mc = rdf_mc.Define("exposure_event_weight", [pot_weight](){ return pot_weight; });
                    campaign_dataset.dataframes[mc_key].push_back(rdf_mc);
                }
            }
        }
    }

public:
    DatasetLoader(const ConfigurationManager& config_manager, const VariableManager& varibale_manager) :
        config_manager_(config_manager), variable_manager_(varibale_manager) {}

    CampaignDataset LoadRuns(
        const std::string& beam_key,
        const std::vector<std::string>& runs_to_load,
        bool blinded = true,
        const VariableOptions& variable_options = VariableOptions{}) const {
        CampaignDataset campaign_dataset;

        for (const auto& run_key : runs_to_load) {
            const auto& run_config = config_manager_.GetRunConfig(beam_key, run_key);

            ProcessingConfig processing_config = {
                run_config,
                variable_options,
                blinded
            };
            this->LoadSamples(campaign_dataset, processing_config);
        }
        return campaign_dataset;
    }
};

}

#endif