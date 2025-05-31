#ifndef DATALOADER_H
#define DATALOADER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <fstream>
#include <algorithm>
#include <numeric>
#include <utility>
#include <iostream>
#include <iomanip>
#include <set>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "TString.h"
#include "TChain.h"

#include "DataTypes.h"
#include "VariableManager.h"
#include "ConfigurationManager.h"
#include "Utilities.h"
#include "DefinitionManager.h"

namespace AnalysisFramework {

class DataLoader {
public:
    using RNodePair = std::pair<SampleCategory, ROOT::RDF::RNode>;
    using NominalDataFrameMap = std::map<std::string, RNodePair>;
    using VariationDataFrameMap = std::map<std::string, ROOT::RDF::RNode>;
    using AssociatedVariationMap = std::map<std::string, VariationDataFrameMap>;

    struct LoadedData {
        NominalDataFrameMap nominal_samples;
        AssociatedVariationMap associated_detvars;
        double data_pot;
    };

    DataLoader(const ConfigurationManager& cfg_mgr) :
        config_manager_(cfg_mgr),
        variable_manager_(),
        definition_manager_(variable_manager_)
    {
    }

    struct LoadRunsParameterSet {
        std::string beam_key;
        std::vector<std::string> runs_to_load;
        bool blinded = true;
        VariableOptions variable_options = VariableOptions{};
    };

    LoadedData LoadRuns(const LoadRunsParameterSet& params) const {
        LoadedData loaded_data;
        double data_pot = 0.0;
        for (const auto& run_key : params.runs_to_load) {
            const auto& run_config = config_manager_.GetRunConfig(params.beam_key, run_key);
            auto [nominal_dataframes, associated_detvars, run_pot] = this->loadSamples(run_config, params.variable_options, params.blinded);
            for (const auto& [sample_key, rnode_pair] : nominal_dataframes) {
                if (loaded_data.nominal_samples.find(sample_key) == loaded_data.nominal_samples.end()) {
                    loaded_data.nominal_samples[sample_key] = rnode_pair;
                }
            }
            for (const auto& [assoc_key, detvar_map] : associated_detvars) {
                for (const auto& [detvar_key, rnode] : detvar_map) {
                    if (loaded_data.associated_detvars.find(assoc_key) == loaded_data.associated_detvars.end()) {
                        loaded_data.associated_detvars[assoc_key] = VariationDataFrameMap();
                    }
                    if (loaded_data.associated_detvars[assoc_key].find(detvar_key) == loaded_data.associated_detvars[assoc_key].end()) {
                        loaded_data.associated_detvars[assoc_key][detvar_key] = rnode;
                    }
                }
            }
            data_pot += run_pot;
        }
        loaded_data.data_pot = data_pot;
        std::cout << "-- Total Data POT: " << data_pot << std::endl;
        return loaded_data;
    }

private:
    ConfigurationManager config_manager_;
    VariableManager variable_manager_;
    DefinitionManager definition_manager_;

    ROOT::RDF::RNode createDataFrame(
        SampleCategory category,
        const std::string& file_path,
        const VariableOptions& variable_options) const {
        std::vector<std::string> variables = variable_manager_.GetVariables(variable_options, category);
        std::set<std::string> unique_vars(variables.begin(), variables.end());
        variables.assign(unique_vars.begin(), unique_vars.end());
        return ROOT::RDataFrame("nuselection/EventSelectionFilter", file_path, variables);
    }

    std::string buildExclusiveFilter(const std::vector<std::string>& mc_keys,
                                     const std::map<std::string, NominalSampleProperties>& samples) const {
        std::string filter = "true";
        for (const auto& key : mc_keys) {
            auto it = samples.find(key);
            if (it != samples.end() && !it->second.truth_filter.empty()) {
                if (filter == "true") {
                    filter = "!(" + it->second.truth_filter + ")";
                } else {
                    filter += " && !(" + it->second.truth_filter + ")";
                }
            }
        }
        return filter;
    }

    ROOT::RDF::RNode loadDataSample(const std::string& file_path, const VariableOptions& variable_options) const {
        std::cout << "-- Loading data sample from " << file_path << std::endl;
        ROOT::RDF::RNode df = createDataFrame(SampleCategory::kData, file_path, variable_options);
        df = definition_manager_.ProcessNode(df, SampleCategory::kData, variable_options, false);
        df = df.Define("event_weight", [](){ return 1.0; });
        return df;
    }

    ROOT::RDF::RNode loadExternalSample(const std::string& file_path, int sample_triggers, int current_run_triggers, const VariableOptions& variable_options) const {
        std::cout << "-- Loading external sample from " << file_path << std::endl;
        ROOT::RDF::RNode df = createDataFrame(SampleCategory::kExternal, file_path, variable_options);
        df = definition_manager_.ProcessNode(df, SampleCategory::kExternal, variable_options, false);
        double event_weight = 1.0;
        if (sample_triggers > 0 && current_run_triggers > 0) {
            event_weight = static_cast<double>(current_run_triggers) / sample_triggers;
        } 
        df = df.Define("event_weight", [event_weight](){ return event_weight; });
        return df;
    }

    ROOT::RDF::RNode loadAndProcessMCDataFrame(
        const std::string& file_path,
        double sample_pot,
        double current_run_pot,
        const std::string& truth_filter,
        const std::vector<std::string>& exclusion_truth_filters,
        const std::map<std::string, NominalSampleProperties>& all_samples,
        const VariableOptions& variable_options,
        bool is_variation,
        const std::string& sample_key
    ) const {
        std::cout << "-- Loading " << (is_variation ? "detector variation" : "nominal MC") << " sample: " << sample_key << " from " << file_path << std::endl;
        ROOT::RDF::RNode df = createDataFrame(SampleCategory::kMonteCarlo, file_path, variable_options);
        double event_weight = 1.0;
        if (sample_pot > 0 && current_run_pot > 0) {
            event_weight = current_run_pot / sample_pot;
        } 

        df = df.Define("event_weight", [event_weight](){ return event_weight; });
        df = definition_manager_.ProcessNode(df, SampleCategory::kMonteCarlo, variable_options, is_variation);

        if (!truth_filter.empty()) {
            df = df.Filter(truth_filter);
        }
        if (!exclusion_truth_filters.empty()) {
            std::string exclusion_filter = buildExclusiveFilter(exclusion_truth_filters, all_samples);
            if (exclusion_filter != "true") {
                df = df.Filter(exclusion_filter);
            }
        }
        return df;
    }

    std::tuple<NominalDataFrameMap, AssociatedVariationMap, double> loadSamples(
        const RunConfiguration& run_config,
        const VariableOptions& variable_options,
        bool blinded) const {
        NominalDataFrameMap nominal_dataframes;
        AssociatedVariationMap associated_detvars;
        double current_run_pot = 0.0;
        int current_run_triggers = 0;

        auto data_props_iter = std::find_if(
            run_config.sample_props.begin(),
            run_config.sample_props.end(),
            [](const auto& pair) { return pair.second.category == SampleCategory::kData; }
        );

        if (data_props_iter != run_config.sample_props.end()) {
            current_run_pot = data_props_iter->second.pot;
            current_run_triggers = data_props_iter->second.triggers;
        } else if (!blinded) {
            std::cout << "-- Info: No data sample found in run configuration for POT/trigger reference, but analysis is unblinded." << std::endl;
        }


        const auto& base_directory = config_manager_.GetBaseDirectory();

        for (const auto& [sample_key, sample_props] : run_config.sample_props) {
            std::string file_path = base_directory + "/" + sample_props.relative_path;

            if (sample_props.category == SampleCategory::kData) {
                if (!blinded) {
                    ROOT::RDF::RNode df = loadDataSample(file_path, variable_options);
                    nominal_dataframes[sample_key] = {SampleCategory::kData, std::move(df)};
                } 
            } else if (sample_props.category == SampleCategory::kExternal) {
                ROOT::RDF::RNode df = loadExternalSample(file_path, sample_props.triggers, current_run_triggers, variable_options);
                nominal_dataframes[sample_key] = {SampleCategory::kExternal, std::move(df)};
            } else if (sample_props.category == SampleCategory::kMonteCarlo) {
                ROOT::RDF::RNode nominal_df = loadAndProcessMCDataFrame(
                    file_path,
                    sample_props.pot,
                    current_run_pot,
                    sample_props.truth_filter,
                    sample_props.exclusion_truth_filters,
                    run_config.sample_props,
                    variable_options,
                    false,
                    sample_key
                );
                nominal_dataframes[sample_key] = {SampleCategory::kMonteCarlo, std::move(nominal_df)};

                for (const auto& detvar_props : sample_props.detector_variations) {
                    std::string detvar_file_path = base_directory + "/" + detvar_props.relative_path;
                    ROOT::RDF::RNode detvar_df = loadAndProcessMCDataFrame(
                        detvar_file_path,
                        detvar_props.pot,
                        current_run_pot,
                        sample_props.truth_filter,
                        sample_props.exclusion_truth_filters,
                        run_config.sample_props,
                        variable_options,
                        true,
                        detvar_props.sample_key
                    );
                    associated_detvars[sample_key][detvar_props.sample_key] = std::move(detvar_df);
                }
            }
        }
        return {nominal_dataframes, associated_detvars, current_run_pot};
    }
};

}

#endif