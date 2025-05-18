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

    using DataFramesDict = std::map<std::string, std::vector<ROOT::RDF::RNode>>;

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
            auto [run_dataframes, run_pot] = this->loadSamples(run_config, params.variable_options, params.blinded);
            for (const auto& [sample_key, rnode_weight] : run_dataframes) {
                dataframes_dict[sample_key].push_back(rnode_weight);
            }
            data_pot += run_pot;
        }

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
                    ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                    sample_rdf = this->processDataFrame(sample_rdf, &sample_props, variable_options);
                    sample_rdf = sample_rdf.Define("event_weight", [](){ return 1.0; });
                    run_dataframes[sample_key] = {sample_props.type, sample_rdf};
                }
            }
        }

        for (const auto& sample_pair : run_config.sample_props) {
            if (is_sample_external(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& sample_key = sample_pair.first;
                const auto& sample_props = sample_pair.second;
                std::string file_path = base_directory + "/" + sample_props.relative_path;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                sample_rdf = this->processDataFrame(sample_rdf, &sample_props, variable_options);
                double event_weight = 1.0;
                if (sample_props.triggers > 0 && run_triggers > 0) {
                    event_weight = static_cast<double>(run_triggers) / sample_props.triggers;
                }
                sample_rdf = sample_rdf.Define("event_weight", [event_weight](){ return event_weight; });
                run_dataframes[sample_key] = {sample_props.type, sample_rdf};
            }
        }

        for (const auto& sample_pair : run_config.sample_props) {
            if (is_sample_mc(sample_pair.second.type) && !sample_pair.first.empty()) {
                const std::string& sample_key = sample_pair.first;
                const auto& sample_props = sample_pair.second;
                std::string file_path = base_directory + "/" + sample_props.relative_path;
                ROOT::RDF::RNode sample_rdf = this->createDataFrame(&sample_props, file_path, variable_options);
                sample_rdf = this->processDataFrame(sample_rdf, &sample_props, variable_options);
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
                double event_weight = (mc_props.pot > 0 && data_pot > 0) ? (data_pot / mc_props.pot) : 1.0;
                sample_rdf = sample_rdf.Define("event_weight", [event_weight](){ return event_weight; });
                run_dataframes[sample_key] = {sample_props.type, sample_rdf};
            }
        }

        return {run_dataframes, run_pot};
    }

    ROOT::RDF::RNode processEventCategories(ROOT::RDF::RNode df, const SampleType& sample_type) {
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
            [](int nkp, int nkm, int nk0, int nl, int nsp, int ns0, int nsm, int nx0, int nxm, int nom) {
                return nkp + nkm + nk0 + nl + nsp + ns0 + nsm + nx0 + nxm + nom;
            },
            {"mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m", "mcf_nxi_0", "mcf_nxi_m", "mcf_nomega"}
        );

        auto df_strange_multiplicity = df_total_strange.Define("inclusive_strangeness_multiplicity_type",
            [](int ts) {
                if (ts == 0) return 0;
                if (ts == 1) return 1;
                return 2;
            }, {"mcf_strangeness"}
        );

        auto df_with_event_category = df_strange_multiplicity.Define("event_category_val",
            [sample_type](int nu_pdg, int ccnc, int npi_char_true, int npr_true, int str_mult) {
                int cat = 9999;
                if (is_sample_data(sample_type)) {
                    cat = 0;
                } else if (is_sample_external(sample_type)) {
                    cat = 1;
                } else if (isSampleDirt(sample_type)) {
                    cat = 2;
                } else if (is_sample_mc(sample_type)) {
                    bool isnumu = std::abs(nu_pdg) == 14;
                    bool isnue = std::abs(nu_pdg) == 12;
                    bool iscc = (ccnc == 0);
                    bool isnc = (ccnc == 1);

                    if (str_mult > 0) {
                        if (isnumu && iscc) cat = (str_mult == 1) ? 10 : 11;
                        else if (isnue && iscc) cat = (str_mult == 1) ? 12 : 13;
                        else if (isnc) cat = (str_mult == 1) ? 14 : 15;
                        else cat = 19;
                    } else {
                        if (isnumu) {
                            if (iscc) {
                                if (npi_char_true == 0) { if (npr_true == 0) cat = 100; else if (npr_true == 1) cat = 101; else cat = 102; }
                                else if (npi_char_true == 1) { if (npr_true == 0) cat = 103; else if (npr_true == 1) cat = 104; else cat = 105; }
                                else cat = 106;
                            } else { 
                                if (npi_char_true == 0) { if (npr_true == 0) cat = 110; else if (npr_true == 1) cat = 111; else cat = 112; }
                                else if (npi_char_true == 1) { if (npr_true == 0) cat = 113; else if (npr_true == 1) cat = 114; else cat = 115; }
                                else cat = 116;
                            }
                        } else if (isnue) {
                            if (iscc) {
                                if (npi_char_true == 0) { if (npr_true == 0) cat = 200; else if (npr_true == 1) cat = 201; else cat = 202; }
                                else if (npi_char_true == 1) { if (npr_true == 0) cat = 203; else if (npr_true == 1) cat = 204; else cat = 205; }
                                else cat = 206;
                            } else cat = 210; 
                        } else cat = 998; 
                    }
                }
                return cat;
            }, {"nu_pdg", "nu_ccnc", "mc_n_charged_pions_true", "mc_n_protons_true", "inclusive_strangeness_multiplicity_type"}
        );

        return df_with_event_category.Alias("event_category", "event_category_val");
    }

    ROOT::RDF::RNode processNuMuVariables(ROOT::RDF::RNode df, SampleType sample_type) {
        auto df_mu_mask = df.Define("muon_candidate_selection_mask_vec",
            [](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& pid,
            const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist, const ROOT::RVec<int>& gen) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    mask[i] = (ts[i] > 0.8f) && (pid[i] > 0.2f) && (l[i] > 10.f) && (dist[i] < 4.f) && (gen[i] == 2);
                }
                return mask;
            },
            {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "trk_generation_v"}
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
};

}

#endif