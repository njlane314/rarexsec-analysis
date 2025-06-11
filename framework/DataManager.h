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
    struct Params {
        std::string config_file;
        std::string beam_key;
        std::vector<std::string> runs_to_load;
        bool blinded = true;
        VariableOptions variable_options = {};
    };

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
                throw std::runtime_error("Attempt to access null RNode in SampleInfo");
            }
            return *nominal_df_;
        }

        bool isMonteCarlo() const { return type_ == SampleType::kMonteCarlo; }
        
        void setVariations(VariationDataFrameMap vars) { variations_ = std::move(vars); }
        const VariationDataFrameMap& getVariations() const { return variations_; }
    };

    explicit DataManager(const Params& params)
        : config_manager_(params.config_file),
          variable_manager_(),
          definition_manager_(variable_manager_),
          data_pot_(0.0),
          data_triggers_(0L),
          blinded_(params.blinded),
          beam_key_(params.beam_key),
          runs_to_load_(params.runs_to_load) {
        this->loadRuns(beam_key_, runs_to_load_, params.variable_options);
    }

    const std::map<std::string, SampleInfo>& getAllSamples() const { return samples_; }
    
    const VariableManager& getVariableManager() const { return variable_manager_; }

    AssociatedVariationMap getAssociatedVariations() const {
        AssociatedVariationMap all_det_vars;
        for (const auto& [sample_key, sample_info] : samples_) {
            if (sample_info.isMonteCarlo()) {
                all_det_vars[sample_key] = sample_info.getVariations();
            }
        }
        return all_det_vars;
    }

    void snapshotDataFrames(const std::string& selection_key,
                    const std::string& preselection_key,
                    const std::string& output_file,
                    const std::vector<std::string>& columns_to_save) const {
        std::string query = Selection::getSelectionQuery(TString(selection_key.c_str()), TString(preselection_key.c_str())).Data();

        bool first_tree = true;
        ROOT::RDF::RSnapshotOptions opts; 

        for (const auto& [sample_key_str, sample_info_obj] : samples_) {
            ROOT::RDF::RNode current_df_node = sample_info_obj.getDataFrame(); 
            ROOT::RDF::RNode filtered_df_node = query.empty() ? current_df_node : current_df_node.Filter(query);

            std::vector<std::string> final_columns_to_snapshot;
            auto available_columns = filtered_df_node.GetColumnNames(); 

            if (!columns_to_save.empty()) {
                for(const auto& col_req : columns_to_save) {
                    bool found = false;
                    for(const auto& col_avail : available_columns) { 
                        if (col_req == col_avail) { 
                            final_columns_to_snapshot.push_back(col_req);
                            found = true;
                            break;
                        }
                    }
                }
            } else { 
                for(const auto& col_name_obj : available_columns) { 
                    final_columns_to_snapshot.push_back(std::string(col_name_obj.data(), col_name_obj.length()));
                }
                std::sort(final_columns_to_snapshot.begin(), final_columns_to_snapshot.end()); 
            }

            opts.fMode = first_tree ? "RECREATE" : "UPDATE";
            std::string tree_name = sample_key_str; 

            if (!final_columns_to_snapshot.empty()) { 
                filtered_df_node.Snapshot(tree_name, output_file, final_columns_to_snapshot, opts);
            } else if (available_columns.empty() && query.empty()) { 
                filtered_df_node.Snapshot(tree_name, output_file, {}, opts); 
            } 
            first_tree = false;
        }
    }

    const std::string& getBeamKey() const { return beam_key_; }

    const std::vector<std::string>& getRunsToLoad() const { return runs_to_load_; }
    
    double getDataPOT() const { return data_pot_; }

    long getDataTriggers() const { return data_triggers_; }

    bool isBlinded() const { return blinded_; }

private:
    ConfigurationManager config_manager_;
    VariableManager variable_manager_;
    DefinitionManager definition_manager_;
    std::string beam_key_; 
    std::vector<std::string> runs_to_load_;

    std::map<std::string, DataManager::SampleInfo> samples_;
    double data_pot_;
    long data_triggers_;
    bool blinded_;

    struct LoadedData {
        NominalDataFrameMap nominal_samples;
        AssociatedVariationMap associated_detvars;
        double data_pot;
        long data_triggers;
    };

    void loadRuns(const std::string& beam_key_val,
                  const std::vector<std::string>& runs_to_load_val,
                  const VariableOptions& variable_options_val) {
        LoadedData loaded_data;
        loaded_data.data_pot = 0.0;
        loaded_data.data_triggers = 0L;
        for (const auto& run_key : runs_to_load_val) {
            const auto& run_config = config_manager_.getRunConfig(beam_key_val, run_key);
            double current_run_nominal_pot = run_config.nominal_pot;
            long current_run_nominal_triggers = run_config.nominal_triggers;

            auto [nominal_dataframes, associated_detvars] = this->loadSamples(run_config, variable_options_val, current_run_nominal_pot, current_run_nominal_triggers);
            
            for (const auto& [sample_key, rnode_pair] : nominal_dataframes) {
                loaded_data.nominal_samples.emplace(sample_key, rnode_pair);
            }
            for (const auto& [assoc_key, detvar_map] : associated_detvars) {
                auto& target_map = loaded_data.associated_detvars.try_emplace(assoc_key).first->second;
                for (const auto& [detvar_key, rnode] : detvar_map) {
                    target_map.emplace(detvar_key, rnode);
                }
            }

            loaded_data.data_pot += current_run_nominal_pot;
            loaded_data.data_triggers += current_run_nominal_triggers;
        }
        std::cout << "-- Total Data POT: " << loaded_data.data_pot << std::endl;

        samples_.clear();
        for (const auto& [sample_key, rnode_pair] : loaded_data.nominal_samples) {
            VariationDataFrameMap variations_for_sample;
            if (rnode_pair.first == SampleType::kMonteCarlo) {
                 if (auto it = loaded_data.associated_detvars.find(sample_key); it != loaded_data.associated_detvars.end()) {
                    variations_for_sample = std::move(it->second);
                }
            }
            samples_.emplace(sample_key, SampleInfo(rnode_pair.first, rnode_pair.second, std::move(variations_for_sample)));
        }
        data_pot_ = loaded_data.data_pot;
    }

    std::tuple<NominalDataFrameMap, AssociatedVariationMap> loadSamples(const RunConfiguration& run_config,
                                                                        const VariableOptions& variable_options,
                                                                        const double& current_run_pot, 
                                                                        const long& current_run_triggers) const {
        NominalDataFrameMap nominal_dataframes;
        AssociatedVariationMap associated_detvars;

        const auto& base_directory = config_manager_.getBaseDirectory();

        for (const auto& [sample_key, sample_props] : run_config.sample_props) {
            std::string file_path = base_directory + "/" + sample_props.relative_path;

            if (sample_props.sample_type == SampleType::kData) {
                if (!blinded_) {
                    ROOT::RDF::RNode df = this->loadDataSample(file_path, variable_options);
                    nominal_dataframes.emplace(sample_key, RNodePair{SampleType::kData, std::move(df)});
                }
            } else if (sample_props.sample_type == SampleType::kExternal) {
                ROOT::RDF::RNode df = this->loadExternalSample(file_path, sample_props.triggers, current_run_triggers, variable_options);
                nominal_dataframes.emplace(sample_key, RNodePair{SampleType::kExternal, std::move(df)});
            } else if (sample_props.sample_type == SampleType::kMonteCarlo) {
                ROOT::RDF::RNode nominal_df = this->loadMonteCarloSample(
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
                    ROOT::RDF::RNode detvar_df = this->loadMonteCarloSample(
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
        return {nominal_dataframes, associated_detvars};
    }

    ROOT::RDF::RNode createDataFrame(SampleType sample_type,
                                     const std::string& file_path,
                                     const VariableOptions& variable_options) const {
        std::vector<std::string> variables = variable_manager_.getVariables(variable_options, sample_type);
        std::set<std::string> unique_vars_set(variables.begin(), variables.end());
        std::vector<std::string> unique_vars(unique_vars_set.begin(), unique_vars_set.end());
        return ROOT::RDataFrame("nuselection/EventSelectionFilter", file_path, unique_vars);
    }

    std::string buildExclusionFilter(const std::vector<std::string>& mc_keys,
                                     const std::map<std::string, NominalSampleProperties>& samples) const {
        std::string filter_expression = "true"; 
        bool first_condition = true;
        for (const auto& key : mc_keys) {
            auto it = samples.find(key);
            if (it != samples.end() && !it->second.truth_filter.empty()) {
                if (first_condition) {
                    filter_expression = "!(" + it->second.truth_filter + ")";
                    first_condition = false;
                } else {
                    filter_expression += " && !(" + it->second.truth_filter + ")";
                }
            }
        }
        return filter_expression;
    }

    ROOT::RDF::RNode loadDataSample(const std::string& file_path, 
                                    const VariableOptions& variable_options) const {
        std::cout << "-- Loading data: " << file_path << std::endl;
        ROOT::RDF::RNode df = this->createDataFrame(SampleType::kData, file_path, variable_options);
        df = definition_manager_.processNode(df, SampleType::kData, variable_options, false);
        return df.Define("base_event_weight", [](){ return 1.0; }); 
    }

    ROOT::RDF::RNode loadExternalSample(const std::string& file_path, 
                                        int sample_triggers, 
                                        int current_run_triggers, 
                                        const VariableOptions& variable_options) const {
        std::cout << "-- Loading external: " << file_path << std::endl;
        ROOT::RDF::RNode df = this->createDataFrame(SampleType::kExternal, file_path, variable_options);
        df = definition_manager_.processNode(df, SampleType::kExternal, variable_options, false);
        double calculated_event_weight = 1.0;
        if (sample_triggers > 0 && current_run_triggers > 0) { 
            calculated_event_weight = static_cast<double>(current_run_triggers) / sample_triggers;
        }
        return df.Define("base_event_weight", [calculated_event_weight](){ return calculated_event_weight; });
    }

    ROOT::RDF::RNode loadMonteCarloSample(const std::string& file_path,
                                            double sample_pot,
                                            double current_run_pot,
                                            const std::string& truth_filter,
                                            const std::vector<std::string>& exclusion_truth_filters,
                                            const std::map<std::string, NominalSampleProperties>& all_samples, 
                                            const VariableOptions& variable_options,
                                            bool is_variation,
                                            const std::string& sample_key_for_log) const { 
        std::cout << "-- Loading " << (is_variation ? "variation" : "MC") << ": " << sample_key_for_log << " from " << file_path << std::endl;
        ROOT::RDF::RNode df = this->createDataFrame(SampleType::kMonteCarlo, file_path, variable_options);
        
        double calculated_event_weight = 1.0;
        if (sample_pot > 0 && current_run_pot > 0) { 
            calculated_event_weight = current_run_pot / sample_pot;
        }
        df = df.Define("base_event_weight", [calculated_event_weight](){ return calculated_event_weight; });
        
        df = definition_manager_.processNode(df, SampleType::kMonteCarlo, variable_options, is_variation);
        
        if (!truth_filter.empty()) {
            df = df.Filter(truth_filter, ("Truth Filter: " + truth_filter).c_str());
        }
        if (!exclusion_truth_filters.empty()) {
            std::string exclusion_filter_str = buildExclusionFilter(exclusion_truth_filters, all_samples);
            if (exclusion_filter_str != "true" && !exclusion_filter_str.empty()) { 
                df = df.Filter(exclusion_filter_str, "Exclusion Filter");
            }
        }
        return df;
    }
}; 

} 

#endif