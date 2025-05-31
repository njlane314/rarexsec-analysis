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

#include "SampleTypes.h"
#include "VariableManager.h"
#include "ConfigurationManager.h"
#include "Utilities.h"

namespace AnalysisFramework {

class DataLoader {
public:
    DataLoader(const std::string& base_dir = "/exp/uboone/data/users/nlane/analysis/") {
        config_manager_ = ConfigurationManager(base_dir);
        variable_manager_ = VariableManager();
    }

    using DataFramesDict = std::map<std::string, std::pair<SampleType, std::vector<ROOT::RDF::RNode>>>;

    struct Parameters {
        std::string beam_key;
        std::vector<std::string> runs_to_load;
        bool blinded = true;
        VariableOptions variable_options = VariableOptions{};
    };

    std::pair<DataFramesDict, double> LoadRuns(const Parameters& params) const {
        DataFramesDict dataframes_dict;
        double data_pot = 0.0;
        for (const auto& run_key : params.runs_to_load) {
            const auto& run_config = config_manager_.GetRunConfig(params.beam_key, run_key);
            auto [run_dataframes, run_pot_for_this_run_config] = this->loadSamples(run_config, params.variable_options, params.blinded);
            for (const auto& [sample_key, sample_type_df_pair] : run_dataframes) {
                auto [sample_type, df] = sample_type_df_pair;
                if (dataframes_dict.find(sample_key) == dataframes_dict.end()) {
                    dataframes_dict[sample_key] = {sample_type, {df}};
                } else {
                    if (dataframes_dict[sample_key].first != sample_type) {
                        throw std::runtime_error("-- Inconsistent SampleType for sample " + sample_key);
                    }
                    dataframes_dict[sample_key].second.push_back(df);
                }
            }
            data_pot += run_pot_for_this_run_config;
        }
        long long total_entries = 0;
        for (auto& [sample_key, sample_pair_value] : dataframes_dict) {
            long long sample_total_count = 0;
            for (ROOT::RDF::RNode& df_node : sample_pair_value.second) {
                sample_total_count += *df_node.Count();
            }
            total_entries += sample_total_count;
        }
        std::cout << "-- Total entries across all samples: " << total_entries << '\n';
        std::cout << "-- Total Data POT: " << data_pot << std::endl;
        return {dataframes_dict, data_pot};
    }

private:
    ConfigurationManager config_manager_;
    VariableManager variable_manager_;

    ROOT::RDF::RNode createDataFrame(
        const SampleProperties* sample_props,
        const std::string& file_path,
        const VariableOptions& variable_options) const {
        std::vector<std::string> variables = variable_manager_.GetVariables(variable_options, sample_props->type);
        return ROOT::RDataFrame("nuselection/EventSelectionFilter", file_path, variables);
    }

    ROOT::RDF::RNode processDataFrame(
        const SampleProperties* sample_props,
        const VariableOptions& variable_options,
        ROOT::RDF::RNode df) const {
        df = defineEventCategories(df, sample_props->type);
        df = defineNuMuVariables(df, sample_props->type);
        if (is_sample_mc(sample_props->type) && variable_options.load_weights_and_systematics) {
            df = defineNominalCVWeight(df);
            df = defineSingleKnobVariationWeights(df);
        } else if (is_sample_data(sample_props->type) || is_sample_ext(sample_props->type)) {
            if (!df.HasColumn("event_weight_cv") && df.HasColumn("event_weight")) {
                df = df.Alias("event_weight_cv", "event_weight");
            } else if (!df.HasColumn("event_weight_cv")) {
                df = df.Define("event_weight_cv", "1.0");
            }
        }
        return df;
    }

    std::string buildExclusiveFilter(const std::vector<std::string>& mc_keys,
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

    std::pair<std::map<std::string, std::pair<SampleType, ROOT::RDF::RNode>>, double> loadSamples(
        const RunConfiguration& run_config,
        const VariableOptions& variable_options,
        bool blinded) const {
        std::map<std::string, std::pair<SampleType, ROOT::RDF::RNode>> run_dataframes;
        double current_run_pot = 0.0;
        int current_run_triggers = 0;

        auto data_props_iter = std::find_if(
            run_config.sample_props.begin(),
            run_config.sample_props.end(),
            [](const auto& pair) { return is_sample_data(pair.second.type); }
        );

        if (data_props_iter != run_config.sample_props.end()) {
            current_run_pot = data_props_iter->second.pot;
            current_run_triggers = data_props_iter->second.triggers;
        } else if (!blinded) {
            // placeholder for loading data
        }

        const auto& base_directory = config_manager_.GetBaseDirectory();

        if (!blinded && data_props_iter != run_config.sample_props.end()) {
            const std::string& sample_key = data_props_iter->first;
            const auto& sample_props = data_props_iter->second;
            std::string file_path = base_directory + "/" + sample_props.relative_path;
            std::cout << "-- Loading sample: " << sample_key << " from " << file_path << std::endl;
            ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
            sample_rdf = this->processDataFrame(&sample_props, variable_options, sample_rdf);
            sample_rdf = sample_rdf.Define("event_weight", [](){ return 1.0; });
            run_dataframes.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(sample_key),
                std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
            );
        }

        for (const auto& sample_pair : run_config.sample_props) {
            const std::string& sample_key = sample_pair.first;
            const auto& sample_props = sample_pair.second;
            std::string file_path = base_directory + "/" + sample_props.relative_path;

            if (is_sample_ext(sample_props.type) && !sample_key.empty()) {
                std::cout << "-- Loading sample: " << sample_key << " from " << file_path << std::endl;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                sample_rdf = this->processDataFrame(&sample_props, variable_options, sample_rdf);
                double ext_event_weight = 1.0;
                if (sample_props.triggers > 0 && current_run_triggers > 0) {
                    ext_event_weight = static_cast<double>(current_run_triggers) / sample_props.triggers;
                } else if (current_run_triggers == 0 && data_props_iter != run_config.sample_props.end()) {
                    std::cout << "-- Inconsistent trigger count for external sample scaling for " << sample_key << std::endl;
                }
                sample_rdf = sample_rdf.Define("event_weight", [ext_event_weight](){ return ext_event_weight; });
                run_dataframes.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(sample_key),
                    std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
                );
            } else if (is_sample_mc(sample_props.type) && !sample_key.empty()) {
                std::cout << "-- Loading sample: " << sample_key << " from " << file_path << std::endl;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                double pot_scaled_mc_event_weight = 1.0;
                if (sample_props.pot > 0 && current_run_pot > 0) {
                    pot_scaled_mc_event_weight = current_run_pot / sample_props.pot;
                } else if (current_run_pot == 0.0 && data_props_iter != run_config.sample_props.end()) {
                    std::cout << "-- Inconsistent pot count for monte carlo sample scaling for " << sample_key << std::endl;
                }
                sample_rdf = sample_rdf.Define("event_weight", [pot_scaled_mc_event_weight](){ return pot_scaled_mc_event_weight; });
                sample_rdf = this->processDataFrame(&sample_props, variable_options, sample_rdf);
                if (!sample_props.truth_filter.empty()) {
                    sample_rdf = sample_rdf.Filter(sample_props.truth_filter);
                }
                if (!sample_props.exclusion_truth_filters.empty()) {
                    std::string exclusion_filter = this->buildExclusiveFilter(
                        sample_props.exclusion_truth_filters,
                        run_config.sample_props
                    );
                    if (exclusion_filter != "true") {
                        sample_rdf = sample_rdf.Filter(exclusion_filter);
                    }
                }
                run_dataframes.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(sample_key),
                    std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
                );
            }
        }
        return {run_dataframes, current_run_pot};
    }

    ROOT::RDF::RNode defineNominalCVWeight(ROOT::RDF::RNode df) const {
        return df.Define("event_weight_cv", "event_weight * weightSpline * weightTune * ppfx_cv");
    }

    ROOT::RDF::RNode defineSingleKnobVariationWeights(ROOT::RDF::RNode df) const {
        if (!df.HasColumn("event_weight_cv")) {
            return df;
        }
        auto knob_variations = variable_manager_.GetKnobVariations();
        for (const auto& knob : knob_variations) {
            std::string knob_name = knob.first;
            std::string up_var = knob.second.first;
            std::string dn_var = knob.second.second;
            if (df.HasColumn(up_var)) {
                df = df.Define("weight_" + knob_name + "_up", "event_weight_cv * " + up_var);
            }
            if (df.HasColumn(dn_var)) {
                df = df.Define("weight_" + knob_name + "_dn", "event_weight_cv * " + dn_var);
            } 
        }
        std::string single_var = variable_manager_.GetSingleKnobVariation();
        if (!single_var.empty() && df.HasColumn(single_var)) {
            df = df.Define("weight_" + single_var, "event_weight_cv * " + single_var);
        } 
        return df;
    }

    ROOT::RDF::RNode defineEventCategories(ROOT::RDF::RNode df, const SampleType& sample_type) const {
        auto df_with_defs = df;
        bool is_mc = AnalysisFramework::is_sample_mc(sample_type);

        std::vector<std::string> truth_cols_for_cat = {"mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
                                                       "true_nu_vtx_x", "true_nu_vtx_y", "true_nu_vtx_z", "nu_pdg", "ccnc", "interaction"};
        bool has_all_truth_cols_for_cat = true;
        if (is_mc) {
            for(const auto& col : truth_cols_for_cat){
                if(!df.HasColumn(col)){
                    has_all_truth_cols_for_cat = false;
                     std::cerr << "***DataLoader: Missing MC truth column for event category definition: " << col << std::endl;
                    break;
                }
            }
        }

        if (is_mc) {
            bool has_npp = df.HasColumn("mcf_npp"); bool has_npm = df.HasColumn("mcf_npm"); bool has_npr = df.HasColumn("mcf_npr");
            if (has_npp && has_npm) df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", "mcf_npp + mcf_npm");
            else df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", [](){ return -1; });
            if (has_npr) df_with_defs = df_with_defs.Define("mc_n_protons_true", "mcf_npr");
            else df_with_defs = df_with_defs.Define("mc_n_protons_true", [](){ return -1; });

            if (has_all_truth_cols_for_cat) {
                 df_with_defs = df_with_defs.Define("mcf_strangeness", "mcf_nkp + mcf_nkm + mcf_nk0 + mcf_nlambda + mcf_nsigma_p + mcf_nsigma_0 + mcf_nsigma_m");
                 df_with_defs = df_with_defs.Define("inclusive_strangeness_multiplicity_type",
                    [](int ts) { if (ts == 0) return 0; if (ts == 1) return 1; return 2; }, {"mcf_strangeness"});
                 df_with_defs = df_with_defs.Define("is_in_fiducial",
                    "(true_nu_vtx_x > 5.0 && true_nu_vtx_x < 251.0 && true_nu_vtx_y > -110.0 && true_nu_vtx_y < 110.0 && true_nu_vtx_z > 20.0 && true_nu_vtx_z < 986.0 && (true_nu_vtx_z < 675.0 || true_nu_vtx_z > 775.0))");
                df_with_defs = df_with_defs.Define("event_category",
                    [sample_type](bool is_in_fiducial_val, int nu_pdg_val, int ccnc_val, int interaction_type_val, int str_mult_val) {
                        int cat = 9999;
                        if (AnalysisFramework::is_sample_data(sample_type)) cat = 0;
                        else if (AnalysisFramework::is_sample_ext(sample_type)) cat = 1;
                        else if (AnalysisFramework::is_sample_dirt(sample_type)) cat = 2;
                        else if (AnalysisFramework::is_sample_mc(sample_type)) {
                            if (!is_in_fiducial_val) cat = 3;
                            else {
                                bool isnumu = (std::abs(nu_pdg_val) == 14); bool isnue = (std::abs(nu_pdg_val) == 12);
                                bool iscc = (ccnc_val == 0); bool isnc = (ccnc_val == 1);
                                if (isnc) cat = 20;
                                else if (isnue && iscc) cat = 21;
                                else if (isnumu && iscc) {
                                    if (str_mult_val == 1) cat = 10;
                                    else if (str_mult_val > 1) cat = 11;
                                    else if (str_mult_val == 0) {
                                        if (interaction_type_val == 0) cat = 110;      else if (interaction_type_val == 1) cat = 111;      else if (interaction_type_val == 2) cat = 112;      else cat = 113;  } else cat = 998;  } else cat = 998;  }
                        } return cat;
                    }, {"is_in_fiducial", "nu_pdg", "ccnc", "interaction", "inclusive_strangeness_multiplicity_type"});
            } else { 
                df_with_defs = df_with_defs.Define("mcf_strangeness", [](){ return -1;});
                df_with_defs = df_with_defs.Define("inclusive_strangeness_multiplicity_type", [](){ return -1;});
                df_with_defs = df_with_defs.Define("is_in_fiducial", [](){ return false; });
                df_with_defs = df_with_defs.Define("event_category", [sample_type](){
                     if (AnalysisFramework::is_sample_data(sample_type)) return 0;
                     if (AnalysisFramework::is_sample_ext(sample_type)) return 1;
                     if (AnalysisFramework::is_sample_dirt(sample_type)) return 2;
                     return 998; 
                 });
            }
        } else {
            df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", [](){ return -1;})
                                    .Define("mc_n_protons_true", [](){ return -1;})
                                    .Define("mcf_strangeness", [](){ return -1;})
                                    .Define("inclusive_strangeness_multiplicity_type", [](){ return -1;})
                                    .Define("is_in_fiducial", [](){ return false; });
            df_with_defs = df_with_defs.Define("event_category", [sample_type](){
                if (AnalysisFramework::is_sample_data(sample_type)) return 0;
                if (AnalysisFramework::is_sample_ext(sample_type)) return 1;
                return 9999;
            });
        }
        return df_with_defs;
    }

    ROOT::RDF::RNode defineNuMuVariables(ROOT::RDF::RNode df, SampleType sample_type) const {
        std::vector<std::string> required_trk_cols = {"slice_topo_score_v", "slice_id", "trk_score_v",
            "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "trk_start_x_v", "trk_end_x_v",
            "trk_start_y_v", "trk_end_y_v", "trk_start_z_v", "trk_end_z_v", "trk_mcs_muon_mom_v",
            "trk_range_muon_mom_v", "trk_phi_v", "trk_theta_v"};
        bool all_trk_cols_present = true;
        for(const auto& col : required_trk_cols){
            if(!df.HasColumn(col)){ all_trk_cols_present = false; break; }
        }

        if(!all_trk_cols_present){
            std::cerr << "***DataLoader: One or more track columns missing for NuMu processing. Defining dummy NuMu variables." << std::endl;
            return df.Define("nu_slice_topo_score", [](){return -999.f;})
                     .Define("muon_candidate_selection_mask_vec", [](){ return ROOT::RVec<bool>{};})
                     .Define("selected_muon_idx", [](){ return -1;})
                     .Define("selected_muon_length", [](){ return -1.f;})
                     .Define("selected_muon_momentum_range", [](){ return -1.f;})
                     .Define("selected_muon_momentum_mcs", [](){ return -1.f;})
                     .Define("selected_muon_phi", [](){ return -999.f;})
                     .Define("selected_muon_cos_theta", [](){ return -999.f;})
                     .Define("selected_muon_energy", [](){ return -1.f;})
                     .Define("selected_muon_trk_score", [](){ return -1.f;})
                     .Define("selected_muon_llr_pid_score", [](){ return -999.f;})
                     .Define("n_muon_candidates", [](){ return 0;});
        }

        auto df_with_neutrino_slice_score = df.Define("nu_slice_topo_score",
            [](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                return getElementFromVector(all_slice_scores, static_cast<int>(neutrino_slice_id), -999.f);
            }, {"slice_topo_score_v", "slice_id"}
        );

        auto df_mu_mask = df_with_neutrino_slice_score.Define("muon_candidate_selection_mask_vec",
            [](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& pid,
               const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist,
               const ROOT::RVec<float>& start_x, const ROOT::RVec<float>& end_x,
               const ROOT::RVec<float>& start_y, const ROOT::RVec<float>& end_y,
               const ROOT::RVec<float>& start_z, const ROOT::RVec<float>& end_z,
               const ROOT::RVec<float>& mcs_mom, const ROOT::RVec<float>& range_mom) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool fiducial = (getElementFromVector(start_x, i, 0.f) > 5.0 && getElementFromVector(start_x, i, 0.f) < 251.0 &&
                                     getElementFromVector(end_x, i, 0.f) > 5.0 && getElementFromVector(end_x, i, 0.f) < 251.0 &&
                                     getElementFromVector(start_y, i, 0.f) > -110.0 && getElementFromVector(start_y, i, 0.f) < 110.0 &&
                                     getElementFromVector(end_y, i, 0.f) > -110.0 && getElementFromVector(end_y, i, 0.f) < 110.0 &&
                                     getElementFromVector(start_z, i, 0.f) > 20.0 && getElementFromVector(start_z, i, 0.f) < 986.0 &&
                                     getElementFromVector(end_z, i, 0.f) > 20.0 && getElementFromVector(end_z, i, 0.f) < 986.0);
                    float current_range_mom = getElementFromVector(range_mom, i, 0.f);
                    bool quality = (getElementFromVector(l, i, 0.f) > 10.0 && getElementFromVector(dist, i, 5.0f) < 4.0 &&
                                    (current_range_mom > 0 ? std::abs((getElementFromVector(mcs_mom, i, 0.f) - current_range_mom) / current_range_mom) < 0.5 : true));
                    mask[i] = (getElementFromVector(ts, i, 0.f) > 0.8f) && (getElementFromVector(pid, i, 0.f) > 0.2f) && fiducial && quality;
                }
                return mask;
            },
            {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v",
             "trk_start_x_v", "trk_end_x_v", "trk_start_y_v", "trk_end_y_v",
             "trk_start_z_v", "trk_end_z_v", "trk_mcs_muon_mom_v", "trk_range_muon_mom_v"}
        );

        auto df_sel_idx = df_mu_mask.Define("selected_muon_idx",
            [](const ROOT::RVec<float>& l, const ROOT::RVec<bool>& m) {
                return getIndexFromVectorSort(l, m, 0, false);
            }, {"trk_len_v", "muon_candidate_selection_mask_vec"}
        );

        auto df_mu_props = df_sel_idx
            .Define("selected_muon_length", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -1.f); }, {"trk_len_v", "selected_muon_idx"})
            .Define("selected_muon_momentum_range", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -1.f); }, {"trk_range_muon_mom_v", "selected_muon_idx"})
            .Define("selected_muon_momentum_mcs", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -1.f); }, {"trk_mcs_muon_mom_v", "selected_muon_idx"})
            .Define("selected_muon_phi", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -999.f); }, {"trk_phi_v", "selected_muon_idx"})
            .Define("selected_muon_cos_theta", [](const ROOT::RVec<float>& v, int i) { float t = getElementFromVector(v, i, -999.f); return (std::abs(t) < 100.f && std::isfinite(t)) ? std::cos(t) : -999.f; }, {"trk_theta_v", "selected_muon_idx"})
            .Define("selected_muon_energy", [](float mom) { const float M = 0.105658f; return (mom >= 0.f && std::isfinite(mom)) ? std::sqrt(mom * mom + M * M) : -1.f; }, {"selected_muon_momentum_range"})
            .Define("selected_muon_trk_score", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -1.f); }, {"trk_score_v", "selected_muon_idx"})
            .Define("selected_muon_llr_pid_score", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "selected_muon_idx"});

        return df_mu_props
            .Define("n_muon_candidates", [](const ROOT::RVec<bool>& m) { return ROOT::VecOps::Sum(m); }, {"muon_candidate_selection_mask_vec"});
    }
};

}

#endif