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

#include "ROOT/RDataFrame.hxx"
#include "TString.h"
#include "TChain.h"

#include "SampleTypes.h"
#include "VariableManager.h"
#include "ConfigurationManager.h"

namespace AnalysisFramework {

template<typename T_vec, typename T_val = typename T_vec::value_type>
T_val getElementFromVector(const T_vec& vec, int index, T_val default_val = T_val{}) {
    if (index >= 0 && static_cast<size_t>(index) < vec.size()) {
        return vec[index];
    }
    return default_val;
}

inline int getIndexFromVectorSort(const ROOT::RVec<float>& values_vec, const ROOT::RVec<bool>& mask_vec, int n_th_idx = 0, bool asc = false) {
    if (values_vec.empty() || (!mask_vec.empty() && values_vec.size() != mask_vec.size())) {
        return -1;
    }

    std::vector<std::pair<float, int>> masked_values;
    if (!mask_vec.empty()) {
        for (size_t i = 0; i < values_vec.size(); ++i) {
            if (mask_vec[i]) {
                masked_values.push_back({values_vec[i], static_cast<int>(i)});
            }
        }
    } else {
        for (size_t i = 0; i < values_vec.size(); ++i) {
            masked_values.push_back({values_vec[i], static_cast<int>(i)});
        }
    }

    if (masked_values.empty() || n_th_idx < 0 || static_cast<size_t>(n_th_idx) >= masked_values.size()) {
        return -1;
    }

    auto comp = [asc](const auto& a, const auto& b) { return asc ? a.first < b.first : a.first > b.first; };

    std::nth_element(masked_values.begin(), masked_values.begin() + n_th_idx, masked_values.end(), comp);
    return masked_values[n_th_idx].second;
}

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
        std::cout << "\n========================================\n";
        std::cout << "Starting Data Loading Process...\n";
        std::cout << "========================================\n" << std::endl;

        DataFramesDict dataframes_dict;
        double data_pot = 0.0;

        for (const auto& run_key : params.runs_to_load) {
            const auto& run_config = config_manager_.GetRunConfig(params.beam_key, run_key);
            auto [run_dataframes, run_pot] = this->loadSamples(run_config, params.variable_options, params.blinded);
            for (const auto& [sample_key, sample_type_df_pair] : run_dataframes) {
                auto [sample_type, df] = sample_type_df_pair;
                if (dataframes_dict.find(sample_key) == dataframes_dict.end()) {
                    dataframes_dict[sample_key] = {sample_type, {df}};
                } else {
                    if (dataframes_dict[sample_key].first != sample_type) {
                        throw std::runtime_error("Inconsistent SampleType for sample " + sample_key);
                    }
                    dataframes_dict[sample_key].second.push_back(df);
                }
            }
            data_pot += run_pot;
        }

        std::cout << "\n========================================\n";
        std::cout << "Data Loading Finished.\n";
        std::cout << "========================================\n" << std::endl;

        long long total_entries = 0;
        for (auto& [sample_key, sample_pair_value] : dataframes_dict) {
            long long sample_total_count = 0;
            for (ROOT::RDF::RNode& df_node : sample_pair_value.second) { 
                sample_total_count += *df_node.Count(); 
            }
            total_entries += sample_total_count;
        }

        std::cout << "--- Data Loading Summary ---\n";
        
        std::cout << "------------------------------------------------------\n";
        std::cout << "Total entries across all samples: " << total_entries << '\n';
        std::cout << "Total Data POT: " << data_pot << '\n';
        std::cout << "------------------------------------------------------\n" << std::endl;


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
        df = processEventCategories(df, sample_props->type);
        df = processNuMuVariables(df, sample_props->type);
        df = processBlipVariables(df);
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
        bool blinded) const
    {
        std::map<std::string, std::pair<SampleType, ROOT::RDF::RNode>> run_dataframes;

        auto data_props_iter = std::find_if(
            run_config.sample_props.begin(),
            run_config.sample_props.end(),
            [](const auto& pair) { return is_sample_data(pair.second.type); }
        );

        const auto& run_props = data_props_iter->second;
        double run_pot = run_props.pot;
        int run_triggers = run_props.triggers;

        const auto& base_directory = config_manager_.GetBaseDirectory();

        if (!blinded) {
            for (const auto& sample_pair : run_config.sample_props) {
                if (is_sample_data(sample_pair.second.type) && !sample_pair.first.empty()) {
                    const std::string& sample_key = sample_pair.first;
                    const auto& sample_props = sample_pair.second;
                    std::string file_path = base_directory + "/" + sample_props.relative_path;
                    std::cout << "  -> Loading sample: " << sample_key << std::endl;
                    ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                    sample_rdf = this->processDataFrame(&sample_props, variable_options, sample_rdf);
                    sample_rdf = sample_rdf.Define("event_weight", [](){ return 1.0; });
                    run_dataframes.emplace(
                        std::piecewise_construct,
                        std::forward_as_tuple(sample_key),
                        std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
                    );
                }
            }
        }

        for (const auto& sample_pair : run_config.sample_props) {
            if (is_sample_ext(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& sample_key = sample_pair.first;
                const auto& sample_props = sample_pair.second;
                std::string file_path = base_directory + "/" + sample_props.relative_path;
                std::cout << "  -> Loading sample: " << sample_key << std::endl;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                sample_rdf = this->processDataFrame(&sample_props, variable_options, sample_rdf);
                double event_weight = 1.0;
                if (sample_props.triggers > 0 && run_triggers > 0) {
                    event_weight = static_cast<double>(run_triggers) / sample_props.triggers;
                }
                sample_rdf = sample_rdf.Define("event_weight", [event_weight](){ return event_weight; });
                run_dataframes.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(sample_key),
                    std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
                );
            }
        }

        for (const auto& sample_pair : run_config.sample_props) {
            if (is_sample_mc(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& sample_key = sample_pair.first;
                const auto& sample_props = sample_pair.second;
                std::string file_path = base_directory + "/" + sample_props.relative_path;
                std::cout << "  -> Loading sample: " << sample_key << std::endl;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
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
                double event_weight = (sample_props.pot > 0 && run_pot > 0) ? (run_pot / sample_props.pot) : 1.0;
                sample_rdf = sample_rdf.Define("event_weight", [event_weight](){ return event_weight; });
                run_dataframes.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(sample_key),
                    std::forward_as_tuple(sample_props.type, std::move(sample_rdf))
                );
            }
        }

        return {run_dataframes, run_pot};
    }

    ROOT::RDF::RNode processEventCategories(ROOT::RDF::RNode df, const SampleType& sample_type) const {
        auto df_with_defs = df;
        bool is_mc = is_sample_mc(sample_type);

        if (is_mc) {
            bool has_npp = false, has_npm = false, has_npr = false;
            for (const auto& cn : df.GetColumnNames()) {
                if (cn == "mcf_npp") has_npp = true;
                if (cn == "mcf_npm") has_npm = true;
                if (cn == "mcf_npr") has_npr = true;
            }
            if (has_npp && has_npm) {
                df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", "mcf_npp + mcf_npm");
            } else {
                df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", "return -1;");
            }
            if (has_npr) {
                df_with_defs = df_with_defs.Define("mc_n_protons_true", "mcf_npr");
            } else {
                df_with_defs = df_with_defs.Define("mc_n_protons_true", "return -1;");
            }
        } else {
            df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", "return -1;")
                                    .Define("mc_n_protons_true", "return -1;");
        }

        auto df_total_strange = df_with_defs.Define("mcf_strangeness",
            [](int nkp, int nkm, int nk0, int nl, int nsp, int ns0, int nsm) {
                return nkp + nkm + nk0 + nl + nsp + ns0 + nsm;
            },
            {"mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m"}
        );

        auto df_strange_multiplicity = df_total_strange.Define("inclusive_strangeness_multiplicity_type",
            [](int ts) {
                if (ts == 0) return 0;
                if (ts == 1) return 1;
                return 2;
            }, {"mcf_strangeness"}
        );

        auto df_with_fiducial = df_strange_multiplicity.Define("is_in_fiducial",
            [](float vtx_x, float vtx_y, float vtx_z) {
                return (vtx_x > 5.0 && vtx_x < 251.0 &&
                        vtx_y > -110.0 && vtx_y < 110.0 &&
                        vtx_z > 20.0 && vtx_z < 986.0 &&
                        (vtx_z < 675.0 || vtx_z > 775.0));
            }, {"true_nu_vtx_x", "true_nu_vtx_y", "true_nu_vtx_z"}
        );

        auto df_with_event_category = df_with_fiducial.Define("event_category",
            [sample_type](bool is_in_fiducial, int nu_pdg, int ccnc, int npi_char_true, int npr_true, int str_mult) {
                int cat = 9999;
                if (is_sample_data(sample_type)) {
                    cat = 0;
                } else if (is_sample_ext(sample_type)) {
                    cat = 1;
                } else if (is_sample_dirt(sample_type)) {
                    cat = 2;
                } else if (is_sample_mc(sample_type)) {
                    if (!is_in_fiducial) {
                        cat = 3; 
                    } else {
                        bool isnumu = (std::abs(nu_pdg) == 14);
                        bool isnue = (std::abs(nu_pdg) == 12);
                        bool iscc = (ccnc == 0);
                        bool isnc = (ccnc == 1);

                        if (isnc) {
                            cat = 20;
                        } else if (isnue && iscc) {
                            cat = 21;
                        } else if (isnumu && iscc) {
                            if (str_mult == 1) {
                                cat = 10;
                            } else if (str_mult == 2) {
                                cat = 11;
                            } else if (str_mult == 0) {
                                if (npi_char_true == 0) {
                                    if (npr_true == 0) cat = 100;
                                    else if (npr_true == 1) cat = 101;
                                    else cat = 102;
                                } else if (npi_char_true == 1) {
                                    if (npr_true == 0) cat = 103;
                                    else if (npr_true == 1) cat = 104;
                                    else cat = 105;
                                } else {
                                    cat = 106;
                                }
                            } else {
                                cat = 998;
                            }
                        } else {
                            cat = 998;
                        }
                    }
                }
                return cat;
            }, {"is_in_fiducial", "nu_pdg", "ccnc", "mc_n_charged_pions_true", "mc_n_protons_true", "inclusive_strangeness_multiplicity_type"}
        );

        return df_with_event_category;
    }

    ROOT::RDF::RNode processNuMuVariables(ROOT::RDF::RNode df, SampleType sample_type) const {
        auto df_with_neutrino_slice_score = df.Define("nu_slice_topo_score",
            [](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                if (static_cast<size_t>(neutrino_slice_id) < all_slice_scores.size()) {
                    return all_slice_scores[neutrino_slice_id];
                }
                return -999.f;
            }, {"slice_topo_score_v", "slice_id"}
        );

        auto df_mu_mask = df_with_neutrino_slice_score.Define("muon_candidate_selection_mask_vec",
            [](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& pid,
            const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    mask[i] = (ts[i] > 0.8f) && (pid[i] > 0.2f) && (l[i] > 10.f) && (dist[i] < 4.f);
                }
                return mask;
            },
            {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v"}
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
            .Define("selected_muon_cos_theta", [](const ROOT::RVec<float>& v, int i) { float t = getElementFromVector(v, i, -999.f); return (t > -998.f && std::isfinite(t)) ? std::cos(t) : -999.f; }, {"trk_theta_v", "selected_muon_idx"})
            .Define("selected_muon_energy", [](float mom) { const float M = 0.105658f; return (mom >= 0.f && std::isfinite(mom)) ? std::sqrt(mom * mom + M * M) : -1.f; }, {"selected_muon_momentum_range"})
            .Define("selected_muon_trk_score", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -1.f); }, {"trk_score_v", "selected_muon_idx"})
            .Define("selected_muon_llr_pid_score", [](const ROOT::RVec<float>& v, int i) { return getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "selected_muon_idx"});

        return df_mu_props
            .Define("n_muon_candidates", [](const ROOT::RVec<bool>& m) { return ROOT::VecOps::Sum(m); }, {"muon_candidate_selection_mask_vec"});
    }

    ROOT::RDF::RNode processBlipVariables(ROOT::RDF::RNode df) const {
        bool has_blip_nplanes = false;
        for (const auto& colName : df.GetColumnNames()) {
            if (colName == "blip_NPlanes") {
                has_blip_nplanes = true;
                break;
            }
        }

        if (!has_blip_nplanes) {
            return df;
        }

        auto df_with_planes = df
            .Define("blip_NPlanes_u", [](const ROOT::RVec<int>& v) { return getElementFromVector(v, 0, -1); }, {"blip_NPlanes"})
            .Define("blip_NPlanes_v", [](const ROOT::RVec<int>& v) { return getElementFromVector(v, 1, -1); }, {"blip_NPlanes"})
            .Define("blip_NPlanes_w", [](const ROOT::RVec<int>& v) { return getElementFromVector(v, 2, -1); }, {"blip_NPlanes"});

        return df_with_planes;
    }
};

}

#endif