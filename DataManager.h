#ifndef DATAMANAGER_H
#define DATAMANAGER_H

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
#include <memory>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include "TString.h"
#include "TChain.h"

#include "DataTypes.h"
#include "VariableManager.h"
#include "ConfigurationManager.h"
#include "DefinitionManager.h"
#include "Selection.h"  

namespace AnalysisFramework {

class DataManager {
public:
    using RNodePair = std::pair<SampleType, ROOT::RDF::RNode>;
    using NominalDataFrameMap = std::map<std::string, RNodePair>;
    using VariationDataFrameMap = std::map<std::string, ROOT::RDF::RNode>;
    using AssociatedVariationMap = std::map<std::string, VariationDataFrameMap>;

    class SampleInfo {
    private:
        SampleType type_;
        std::shared_ptr<ROOT::RDF::RNode> nominal_df_;
        VariationDataFrameMap variations_;
    public:
        SampleInfo() : type_(SampleType::kData), nominal_df_(nullptr), variations_() {}
        
        SampleInfo(SampleType type, ROOT::RDF::RNode df,
                   VariationDataFrameMap vars = {}) 
            : type_(type), nominal_df_(std::make_shared<ROOT::RDF::RNode>(std::move(df))), variations_(std::move(vars)) {}
        
        ROOT::RDF::RNode getDataFrame() const {
            if (!nominal_df_) {
                throw std::runtime_error("Attempt to access null RNode");
            }
            return *nominal_df_;
        }
        
        bool isMonteCarlo() const { return type_ == SampleType::kMonteCarlo; }
        const VariationDataFrameMap& getVariations() const { return variations_; } 
        void setVariations(VariationDataFrameMap vars) { variations_ = std::move(vars); } 
    };

    DataManager(const std::string& config_file,
                const std::string& beam_key,
                const std::vector<std::string>& runs_to_load,
                bool blinded,
                const VariableOptions& variable_options)
        : config_manager_(config_file), variable_manager_(), definition_manager_(variable_manager_) {
        LoadRunsParameterSet params = {beam_key, runs_to_load, blinded, variable_options};
        loadRuns(params);
    }

    const std::map<std::string, SampleInfo>& getAllSamples() const { return samples_; }
    double getDataPOT() const { return data_pot_; }

    void Save(const std::string& selection_key,
              const std::string& preselection_key,
              const std::string& output_file,
              const std::vector<std::string>& columns_to_save = {}) const;

private:
    ConfigurationManager config_manager_;
    VariableManager variable_manager_;
    DefinitionManager definition_manager_;

    std::map<std::string, DataManager::SampleInfo> samples_;
    double data_pot_;

    struct LoadRunsParameterSet {
        std::string beam_key;
        std::vector<std::string> runs_to_load;
        bool blinded = true;
        VariableOptions variable_options = VariableOptions{};
    };

    struct LoadedData {
        NominalDataFrameMap nominal_samples;
        AssociatedVariationMap associated_detvars;
        double data_pot;
    };

    void loadRuns(const LoadRunsParameterSet& params) {
        LoadedData loaded_data;
        double data_pot = 0.0;
        for (const auto& run_key : params.runs_to_load) {
            const auto& run_config = config_manager_.GetRunConfig(params.beam_key, run_key);
            auto [nominal_dataframes, associated_detvars, run_pot] = loadSamples(run_config, params.variable_options, params.blinded);
            for (const auto& [sample_key, rnode_pair] : nominal_dataframes) {
                auto it = loaded_data.nominal_samples.find(sample_key);
                if (it == loaded_data.nominal_samples.end()) {
                    loaded_data.nominal_samples.emplace(sample_key, std::move(rnode_pair));
                }
            }
            for (const auto& [assoc_key, detvar_map] : associated_detvars) {
                auto outer_it = loaded_data.associated_detvars.find(assoc_key);
                if (outer_it == loaded_data.associated_detvars.end()) {
                    VariationDataFrameMap new_map;
                    for (const auto& [detvar_key, rnode] : detvar_map) {
                        new_map.emplace(detvar_key, std::move(rnode));
                    }
                    loaded_data.associated_detvars.emplace(assoc_key, std::move(new_map));
                } else {
                    for (const auto& [detvar_key, rnode] : detvar_map) {
                        auto inner_it = outer_it->second.find(detvar_key);
                        if (inner_it == outer_it->second.end()) {
                            outer_it->second.emplace(detvar_key, std::move(rnode));
                        }
                    }
                }
            }
            data_pot += run_pot;
        }
        loaded_data.data_pot = data_pot;
        std::cout << "-- Total Data POT: " << data_pot << std::endl;

        samples_.clear();
        for (const auto& [sample_key, rnode_pair] : loaded_data.nominal_samples) {
            SampleInfo info(rnode_pair.first, std::move(rnode_pair.second), {});
            if (rnode_pair.first == SampleType::kMonteCarlo) {
                info.setVariations(std::move(loaded_data.associated_detvars[sample_key]));
            }
            samples_.emplace(sample_key, std::move(info));
        }
        data_pot_ = loaded_data.data_pot;
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
            [](const auto& pair) { return pair.second.category == SampleType::kData; }
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

            if (sample_props.category == SampleType::kData) {
                if (!blinded) {
                    ROOT::RDF::RNode df = loadDataSample(file_path, variable_options);
                    nominal_dataframes.emplace(sample_key, RNodePair{SampleType::kData, std::move(df)});
                }
            } else if (sample_props.category == SampleType::kExternal) {
                ROOT::RDF::RNode df = loadExternalSample(file_path, sample_props.triggers, current_run_triggers, variable_options);
                nominal_dataframes.emplace(sample_key, RNodePair{SampleType::kExternal, std::move(df)});
            } else if (sample_props.category == SampleType::kMonteCarlo) {
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
                nominal_dataframes.emplace(sample_key, RNodePair{SampleType::kMonteCarlo, std::move(nominal_df)});

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
                    associated_detvars[sample_key].emplace(detvar_props.sample_key, std::move(detvar_df));
                }
            }
        }
        return {nominal_dataframes, associated_detvars, current_run_pot};
    }

    ROOT::RDF::RNode createDataFrame(
        SampleType category,
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
        ROOT::RDF::RNode df = createDataFrame(SampleType::kData, file_path, variable_options);
        df = definition_manager_.ProcessNode(df, SampleType::kData, variable_options, false);
        df = df.Define("event_weight", [](){ return 1.0; });
        return df;
    }

    ROOT::RDF::RNode loadExternalSample(const std::string& file_path, int sample_triggers, int current_run_triggers, const VariableOptions& variable_options) const {
        std::cout << "-- Loading external sample from " << file_path << std::endl;
        ROOT::RDF::RNode df = createDataFrame(SampleType::kExternal, file_path, variable_options);
        df = definition_manager_.ProcessNode(df, SampleType::kExternal, variable_options, false);
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
        const std::string& sample_key) const {
        std::cout << "-- Loading " << (is_variation ? "detector variation" : "nominal MC") << " sample: " << sample_key << " from " << file_path << std::endl;
        ROOT::RDF::RNode df = createDataFrame(SampleType::kMonteCarlo, file_path, variable_options);
        double event_weight = 1.0;
        if (sample_pot > 0 && current_run_pot > 0) {
            event_weight = current_run_pot / sample_pot;
        }
        df = df.Define("event_weight", [event_weight](){ return event_weight; });
        df = definition_manager_.ProcessNode(df, SampleType::kMonteCarlo, variable_options, is_variation);
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
};

void DataManager::Save(const std::string& selection_key,
                       const std::string& preselection_key,
                       const std::string& output_file,
                       const std::vector<std::string>& columns_to_save) const {
    if (output_file.empty()) {
        throw std::invalid_argument("Output file name cannot be empty.");
    }

    std::string query = Selection::getSelectionQuery(TString(selection_key.c_str()), TString(preselection_key.c_str())).Data();
    if (query.empty()) {
        throw std::runtime_error("Selection query is empty");
    }

    bool first_tree = true;
    for (const auto& [sample_key, sample_info] : samples_) {
        auto df = sample_info.getDataFrame();
        auto filtered_df = df.Filter(query);

        std::vector<std::string> final_columns_to_snapshot;
        if (!columns_to_save.empty()) {
            final_columns_to_snapshot = columns_to_save;
        } else {
            final_columns_to_snapshot = filtered_df.GetColumnNames();
            std::sort(final_columns_to_snapshot.begin(), final_columns_to_snapshot.end());
        }

        ROOT::RDF::RSnapshotOptions opts;
        opts.fMode = first_tree ? "RECREATE" : "UPDATE";

        std::string tree_name = sample_key;
        filtered_df.Snapshot(tree_name, output_file, final_columns_to_snapshot, opts);
        first_tree = false;
    }
}

}

#endif // DATAMANAGER_H