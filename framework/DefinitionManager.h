#ifndef DEFINITIONMANAGER_H
#define DEFINITIONMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <utility>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <set>
#include <functional>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

#include "DataTypes.h"
#include "VariableManager.h"

namespace AnalysisFramework {

class DefinitionManager {
public:
    DefinitionManager(const VariableManager& var_mgr) : variable_manager_(var_mgr) {}

    ROOT::RDF::RNode processNode(ROOT::RDF::RNode df, SampleType sample_type,
                                 const VariableOptions& variable_options, bool is_variation) const {
        df = this->defineAnalysisChannels(df, sample_type);
        df = this->defineNuMuVariables(df, sample_type);

        if (sample_type == SampleType::kMonteCarlo) {
            df = this->defineNominalWeight(df);
            df = this->defineKnobVariationWeights(df);
        } else if (sample_type == SampleType::kData || sample_type == SampleType::kExternal) {
            if (!df.HasColumn("central_value_weight") && df.HasColumn("base_event_weight")) {
                df = df.Alias("central_value_weight", "base_event_weight");
            } else if (!df.HasColumn("central_value_weight")) {
                df = df.Define("central_value_weight", "1.0");
            }
        }
        
        df = df.Define("constant_0_5", "0.5"); 
        
        return df;
    }

private:
    const VariableManager& variable_manager_;

    template<typename T_vec, typename T_val = typename T_vec::value_type>
    T_val getElementFromVector(const T_vec& vec, int index, T_val default_val = T_val{}) const {
        if (index >= 0 && static_cast<size_t>(index) < vec.size()) {
            return vec[index];
        }
        return default_val;
    }

    inline int getIndexFromVectorSort(const ROOT::RVec<float>& values_vec, const ROOT::RVec<bool>& mask_vec, int n_th_idx = 0, bool asc = false) const {
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

    ROOT::RDF::RNode defineNominalWeight(ROOT::RDF::RNode df) const {
        auto standard_weight_calculator = [](double w, float w_spline, float w_tune) {
            double final_weight = w;
            if (std::isfinite(w_spline) && w_spline > 0) {
                final_weight *= w_spline;
            }
            if (std::isfinite(w_tune) && w_tune > 0) {
                final_weight *= w_tune;
            }
            if (!std::isfinite(final_weight) || final_weight < 0) {
                return 1.0;
            }
            return final_weight;
        };

        return df.Define("central_value_weight", standard_weight_calculator, {"base_event_weight", "weightSpline", "weightTune"});
    }

    ROOT::RDF::RNode defineKnobVariationWeights(ROOT::RDF::RNode df) const {
        if (!df.HasColumn("central_value_weight")) {
            return df;
        }
        auto knob_variations = variable_manager_.getKnobVariations();
        for (const auto& knob : knob_variations) {
            std::string knob_name = knob.first;
            std::string up_var = knob.second.first;
            std::string dn_var = knob.second.second;
            if (df.HasColumn(up_var)) {
                df = df.Define("weight_" + knob_name + "_up", "central_value_weight * " + up_var);
            }
            if (df.HasColumn(dn_var)) {
                df = df.Define("weight_" + knob_name + "_dn", "central_value_weight * " + dn_var);
            }
        }
        std::string single_var = variable_manager_.getSingleKnobVariation();
        if (!single_var.empty() && df.HasColumn(single_var)) {
            df = df.Define("weight_" + single_var, "central_value_weight * " + single_var);
        }
        return df;
    }

    ROOT::RDF::RNode defineAnalysisChannels(ROOT::RDF::RNode df, SampleType sample_type) const {
        ROOT::RDF::RNode df_with_defs = df;
        bool is_mc = (sample_type == SampleType::kMonteCarlo);

        std::vector<std::string> truth_cols = {"mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
                                               "true_nu_vtx_x", "nu_pdg", "ccnc", "interaction", "mcf_npp", "mcf_npm", "mcf_npr"};
        bool has_all_truth_cols = is_mc;
        if (is_mc) {
            for(const auto& col : truth_cols){ if(!df.HasColumn(col)){ has_all_truth_cols = false; break; } }
        }

        if (is_mc && has_all_truth_cols) {
            df_with_defs = df_with_defs.Define("in_fv", "(true_nu_vtx_x > 5.0 && true_nu_vtx_x < 251.0 && true_nu_vtx_y > -110.0 && true_nu_vtx_y < 110.0 && true_nu_vtx_z > 20.0 && true_nu_vtx_z < 986.0)");
            df_with_defs = df_with_defs.Define("mcf_strangeness", "mcf_nkp + mcf_nkm + mcf_nk0 + mcf_nlambda + mcf_nsigma_p + mcf_nsigma_0 + mcf_nsigma_m");
            df_with_defs = df_with_defs.Define("mc_n_pions", "mcf_npp + mcf_npm");
            df_with_defs = df_with_defs.Define("mc_n_protons", "mcf_npr");

            df_with_defs = df_with_defs.Define("inclusive_strange_channels",
                [](bool is_fid, int nu, int cc, int strange, int n_pi, int n_p) {
                    if (!is_fid) return 98;
                    if (cc == 1) return 31;
                    if (std::abs(nu) == 12 && cc == 0) return 30;

                    if (std::abs(nu) == 14 && cc == 0) {
                        if (strange == 1) return 10;
                        if (strange > 1) return 11;

                        if (n_pi == 0) {
                            if (n_p == 1) return 20;
                            return 21;
                        }
                        if (n_pi == 1) return 22;
                        return 23;
                    }
                    return 99;
                }, {"in_fv", "nu_pdg", "ccnc", "mcf_strangeness", "mc_n_pions", "mc_n_protons"});

            df_with_defs = df_with_defs.Define("exclusive_strange_channels", 
                [](bool is_fid, int nu, int cc, int strange, int n_kp, int n_km, int n_k0, int n_lambda, int n_sigma_p, int n_sigma_0, int n_sigma_m) {
                    if (!is_fid) return 98; 
                    if (cc == 1) return 31; 
                    if (std::abs(nu) == 12 && cc == 0) return 30; 

                    if (std::abs(nu) == 14 && cc == 0) { 
                        if ((n_kp == 1 || n_km == 1) && strange == 1) return 50; 
                        if (n_k0 == 1 && strange == 1) return 51; 
                        if (n_lambda == 1 && strange == 1) return 52; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && strange == 1) return 53; 
                        
                        if (n_lambda == 1 && (n_kp == 1 || n_km == 1) && strange == 2) return 54; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && n_k0 == 1 && strange == 2) return 55; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && (n_kp == 1 || n_km == 1) && strange == 2) return 56;
                        if (n_lambda == 1 && n_k0 == 1) return 57;
                        if (n_kp == 1 && n_km == 1) return 58;

                        if (strange >= 2) return 59;

                        return 32;
                    }
                    return 99;
                }, {"in_fv", "nu_pdg", "ccnc", "mcf_strangeness", "mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m"});
        } else {
            df_with_defs = df_with_defs.Define("inclusive_strange_channels", [sample_type](){
                if (sample_type == SampleType::kData) return 0;
                if (sample_type == SampleType::kExternal) return 1;
                if (sample_type == SampleType::kDirt) return 2;
                return 99;
            });
            df_with_defs = df_with_defs.Define("exclusive_strange_channels", [sample_type](){ 
                if (sample_type == SampleType::kData) return 0;
                if (sample_type == SampleType::kExternal) return 1;
                if (sample_type == SampleType::kDirt) return 2;
                return 99;
            });
        }
        return df_with_defs;
    }

    ROOT::RDF::RNode defineNuMuVariables(ROOT::RDF::RNode df, SampleType sample_type) const {
        ROOT::RDF::RNode df_with_defs = df;
        std::vector<std::string> track_cols = {"slice_topo_score_v", "slice_id", "trk_score_v",
            "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "trk_range_muon_mom_v", "trk_phi_v", 
            "trk_theta_v", "n_tracks", "n_showers", "trk_end_x_v", "trk_end_y_v", "trk_end_z_v"
        };

        bool has_track_cols = true;
        for(const auto& col : track_cols){
            if(!df_with_defs.HasColumn(col)){ has_track_cols = false; break; }
        }
        
        if (has_track_cols) {
            df_with_defs  = df_with_defs.Define("nu_slice_topo_score",
                [this](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                    return this->getElementFromVector(all_slice_scores, static_cast<int>(neutrino_slice_id), -999.f);
                }, {"slice_topo_score_v", "slice_id"}
            );

            if (df_with_defs.HasColumn("pfp_generation_v")) {
                df_with_defs = df_with_defs.Define("n_pfp_gen_0",
                    [](const ROOT::RVec<unsigned int>& generations) {
                        return ROOT::VecOps::Sum(generations == 0);
                    }, {"pfp_generation_v"});

                df_with_defs = df_with_defs.Define("n_pfp_gen_1",
                    [](const ROOT::RVec<unsigned int>& generations) {
                        return ROOT::VecOps::Sum(generations == 1);
                    }, {"pfp_generation_v"});

                df_with_defs = df_with_defs.Define("n_pfp_gen_2",
                    [](const ROOT::RVec<unsigned int>& generations) {
                        return ROOT::VecOps::Sum(generations == 2);
                    }, {"pfp_generation_v"});

                df_with_defs = df_with_defs.Define("n_pfp_gen_3",
                    [](const ROOT::RVec<unsigned int>& generations) {
                        return ROOT::VecOps::Sum(generations == 3);
                    }, {"pfp_generation_v"});

                df_with_defs = df_with_defs.Define("n_pfp_gen_4",
                    [](const ROOT::RVec<unsigned int>& generations) {
                        return ROOT::VecOps::Sum(generations == 4);
                    }, {"pfp_generation_v"});
            
                auto create_muon_mask = [this](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& pid,
                        const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist, const ROOT::RVec<unsigned int>& gen) {
                        ROOT::RVec<bool> mask(ts.size());
                        for (size_t i = 0; i < ts.size(); ++i) {
                            bool quality = (this->getElementFromVector(l, i, 0.f) > 10.0 && this->getElementFromVector(dist, i, 5.0f) < 4.0);
                            bool generation_cut = (this->getElementFromVector(gen, i, 0) == 2);
                            mask[i] = (this->getElementFromVector(ts, i, 0.f) > 0.8f) && (this->getElementFromVector(pid, i, 0.f) > 0.2f && quality && generation_cut);
                        }
                        return mask;
                    };
                df_with_defs = df_with_defs.Define("muon_candidates_mask", create_muon_mask,
                    {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "pfp_generation_v"}
                );

                df_with_defs = df_with_defs.Define("selected_muon_idx",
                    [this](const ROOT::RVec<float>& l, const ROOT::RVec<bool>& m) {
                        return this->getIndexFromVectorSort(l, m, 0, false);
                    }, {"trk_len_v", "muon_candidates_mask"}
                );

                auto create_proton_mask = [this](
                    const ROOT::RVec<float>& track_scores,
                    const ROOT::RVec<float>& pid_scores,
                    const ROOT::RVec<float>& end_x,
                    const ROOT::RVec<float>& end_y,
                    const ROOT::RVec<float>& end_z,
                    const ROOT::RVec<unsigned int>& gen,
                    const ROOT::RVec<float>& dist,
                    int muon_idx
                ) {
                    ROOT::RVec<bool> proton_mask(track_scores.size(), false);
                    for (size_t i = 0; i < track_scores.size(); ++i) {
                        if (static_cast<int>(i) == muon_idx) continue;

                        bool is_neutrino_daughter = (this->getElementFromVector(gen, i, 0) == 2);
                        bool is_track_like = this->getElementFromVector(track_scores, i, 0.f) > 0.5;
                        bool is_proton_like = this->getElementFromVector(pid_scores, i, 1.f) < 0.2;
                        bool is_close_to_nu_vtx = this->getElementFromVector(dist, i, 5.0f) < 1.0;

                        if (is_neutrino_daughter && is_track_like && is_proton_like && is_close_to_nu_vtx) {
                            proton_mask[i] = true;
                        }
                    }
                    return proton_mask;
                };

                df_with_defs = df_with_defs.Define("proton_candidates_mask", create_proton_mask,
                    {"trk_score_v", "trk_llr_pid_score_v", "trk_end_x_v", "trk_end_y_v", "trk_end_z_v", "pfp_generation_v", "trk_distance_v", "selected_muon_idx"});

                df_with_defs = df_with_defs.Define("n_reco_protons", "ROOT::VecOps::Sum(proton_candidates_mask)");

                auto count_pions_updated = [this](
                    const ROOT::RVec<float>& track_scores,
                    const ROOT::RVec<float>& pid_scores,
                    const ROOT::RVec<unsigned int>& gen,
                    const ROOT::RVec<float>& dist,
                    int muon_idx
                ) {
                    int pion_count = 0;
                    for (size_t i = 0; i < pid_scores.size(); ++i) {
                        if (static_cast<int>(i) == muon_idx) continue;
                        
                        bool is_neutrino_daughter = (this->getElementFromVector(gen, i, 0) == 2);
                        bool is_track_like = this->getElementFromVector(pid_scores, i, 0.f) > 0.5;
                        bool is_close_to_nu_vtx = this->getElementFromVector(dist, i, 5.0f) < 1.0;
                        if (is_neutrino_daughter && is_track_like && is_close_to_nu_vtx && this->getElementFromVector(pid_scores, i, 0.f) > 0.2) {
                            pion_count++;
                        }
                    }
                    return pion_count;
                };

                df_with_defs = df_with_defs.Define("n_reco_pions", count_pions_updated,
                    {"trk_score_v", "trk_llr_pid_score_v", "pfp_generation_v", "trk_distance_v", "selected_muon_idx"});
                
                df_with_defs = df_with_defs.Define("is_0p0pi", "(n_reco_protons == 0 && n_reco_pions == 0 && n_pfp_gen_2 == 1 && n_pfp_gen_3  == 0)");
                df_with_defs = df_with_defs.Define("is_1p0pi", "(n_reco_protons == 1 && n_reco_pions == 0 && n_pfp_gen_2 == 2 && n_pfp_gen_3  == 0)");
                df_with_defs = df_with_defs.Define("is_Np0pi", "(n_reco_protons > 1 && n_reco_pions == 0 && n_pfp_gen_2 > 2 && n_pfp_gen_3  == 0)");
                df_with_defs = df_with_defs.Define("is_1pi", "(n_reco_pions == 1 && n_reco_protons == 0 && n_pfp_gen_2 == 2 && n_pfp_gen_3  == 0)");
                df_with_defs = df_with_defs.Define("is_1p1pi", "(n_reco_pions == 1 && n_reco_protons == 1 && n_pfp_gen_2 == 3 && n_pfp_gen_3  == 0)");
            }

            df_with_defs = df_with_defs
                .Define("selected_muon_length", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_len_v", "selected_muon_idx"})
                .Define("selected_muon_momentum_range", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_range_muon_mom_v", "selected_muon_idx"})
                .Define("selected_muon_phi", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -999.f); }, {"trk_phi_v", "selected_muon_idx"})
                .Define("selected_muon_cos_theta", [this](const ROOT::RVec<float>& v, int i) { float t = this->getElementFromVector(v, i, -999.f); return (std::abs(t) < 100.f && std::isfinite(t)) ? std::cos(t) : -999.f; }, {"trk_theta_v", "selected_muon_idx"})
                .Define("selected_muon_trk_score", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_score_v", "selected_muon_idx"})
                .Define("selected_muon_llr_pid_score", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "selected_muon_idx"});

            df_with_defs = df_with_defs.Define("n_muon_candidates", [](const ROOT::RVec<bool>& m) { return ROOT::VecOps::Sum(m); }, {"muon_candidates_mask"});
            df_with_defs = df_with_defs.Define("has_muon_candidate", [](const ROOT::RVec<bool>& m) { return static_cast<int>(ROOT::VecOps::Any(m)); }, {"muon_candidates_mask"});
        }
        return df_with_defs;
    }
};

}

#endif