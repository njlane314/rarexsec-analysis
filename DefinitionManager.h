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

    ROOT::RDF::RNode ProcessNode(ROOT::RDF::RNode df, SampleType category,
                                 const VariableOptions& variable_options, bool is_variation) const {
        df = defineEventCategories(df, category);
        df = defineNuMuVariables(df, category);

        if (category == SampleType::kMonteCarlo) {
            if (!is_variation && variable_options.load_weights_and_systematics) {
                df = defineNominalCVWeight(df);
                df = defineSingleKnobVariationWeights(df);
            } else if (is_variation) {
                df = defineNominalCVWeight(df);
            }
        } else if (category == SampleType::kData || category == SampleType::kExternal) {
            if (!df.HasColumn("event_weight_cv") && df.HasColumn("event_weight")) {
                df = df.Alias("event_weight_cv", "event_weight");
            } else if (!df.HasColumn("event_weight_cv")) {
                df = df.Define("event_weight_cv", "1.0");
            }
        }
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

    ROOT::RDF::RNode defineEventCategories(ROOT::RDF::RNode df, SampleType category) const {
        auto df_with_defs = df;
        bool is_mc = (category == SampleType::kMonteCarlo);

        std::vector<std::string> truth_cols_for_cat = {"mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
                                                       "true_nu_vtx_x", "true_nu_vtx_y", "true_nu_vtx_z", "nu_pdg", "ccnc", "interaction"};
        bool has_all_truth_cols_for_cat = true;
        if (is_mc) {
            for(const auto& col : truth_cols_for_cat){
                if(!df.HasColumn(col)){
                    has_all_truth_cols_for_cat = false;
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
                    [category](bool is_in_fiducial_val, int nu_pdg_val, int ccnc_val, int interaction_type_val, int str_mult_val) {
                        int cat = 9999;
                        if (category == SampleType::kData) cat = 0;
                        else if (category == SampleType::kExternal) cat = 1;
                        else if (category == SampleType::kMonteCarlo) {
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
                                        if (interaction_type_val == 0) cat = 110;
                                        else if (interaction_type_val == 1) cat = 111;
                                        else if (interaction_type_val == 2) cat = 112;
                                        else cat = 113;
                                    } else cat = 998;
                                } else cat = 998;
                            }
                        }
                        return cat;
                    }, {"is_in_fiducial", "nu_pdg", "ccnc", "interaction", "inclusive_strangeness_multiplicity_type"});
            } else {
                df_with_defs = df_with_defs.Define("mcf_strangeness", [](){ return -1;});
                df_with_defs = df_with_defs.Define("inclusive_strangeness_multiplicity_type", [](){ return -1;});
                df_with_defs = df_with_defs.Define("is_in_fiducial", [](){ return false; });
                df_with_defs = df_with_defs.Define("event_category", [category](){
                     if (category == SampleType::kData) return 0;
                     if (category == SampleType::kExternal) return 1;
                     return 998;
                 });
            }
        } else {
            df_with_defs = df_with_defs.Define("mc_n_charged_pions_true", [](){ return -1;})
                                    .Define("mc_n_protons_true", [](){ return -1;})
                                    .Define("mcf_strangeness", [](){ return -1;})
                                    .Define("inclusive_strangeness_multiplicity_type", [](){ return -1;})
                                    .Define("is_in_fiducial", [](){ return false; });
            df_with_defs = df_with_defs.Define("event_category", [category](){
                if (category == SampleType::kData) return 0;
                if (category == SampleType::kExternal) return 1;
                return 9999;
            });
        }
        return df_with_defs;
    }

    ROOT::RDF::RNode defineNuMuVariables(ROOT::RDF::RNode df, SampleType category) const {
        std::vector<std::string> required_trk_cols = {"slice_topo_score_v", "slice_id", "trk_score_v",
            "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "trk_start_x_v", "trk_end_x_v",
            "trk_start_y_v", "trk_end_y_v", "trk_start_z_v", "trk_end_z_v", "trk_mcs_muon_mom_v",
            "trk_range_muon_mom_v", "trk_phi_v", "trk_theta_v"};
        bool all_trk_cols_present = true;
        for(const auto& col : required_trk_cols){
            if(!df.HasColumn(col)){ all_trk_cols_present = false; break; }
        }

        if(!all_trk_cols_present){
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
            [this](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                return this->getElementFromVector(all_slice_scores, static_cast<int>(neutrino_slice_id), -999.f);
            }, {"slice_topo_score_v", "slice_id"}
        );

        auto df_mu_mask = df_with_neutrino_slice_score.Define("muon_candidate_selection_mask_vec",
            [this](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& pid,
                   const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist,
                   const ROOT::RVec<float>& start_x, const ROOT::RVec<float>& end_x,
                   const ROOT::RVec<float>& start_y, const ROOT::RVec<float>& end_y,
                   const ROOT::RVec<float>& start_z, const ROOT::RVec<float>& end_z,
                   const ROOT::RVec<float>& mcs_mom, const ROOT::RVec<float>& range_mom) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool fiducial = (this->getElementFromVector(start_x, i, 0.f) > 5.0 && this->getElementFromVector(start_x, i, 0.f) < 251.0 &&
                                     this->getElementFromVector(end_x, i, 0.f) > 5.0 && this->getElementFromVector(end_x, i, 0.f) < 251.0 &&
                                     this->getElementFromVector(start_y, i, 0.f) > -110.0 && this->getElementFromVector(start_y, i, 0.f) < 110.0 &&
                                     this->getElementFromVector(end_y, i, 0.f) > -110.0 && this->getElementFromVector(end_y, i, 0.f) < 110.0 &&
                                     this->getElementFromVector(start_z, i, 0.f) > 20.0 && this->getElementFromVector(start_z, i, 0.f) < 986.0 &&
                                     this->getElementFromVector(end_z, i, 0.f) > 20.0 && this->getElementFromVector(end_z, i, 0.f) < 986.0);
                    float current_range_mom = this->getElementFromVector(range_mom, i, 0.f);
                    bool quality = (this->getElementFromVector(l, i, 0.f) > 10.0 && this->getElementFromVector(dist, i, 5.0f) < 4.0 &&
                                    (current_range_mom > 0 ? std::abs((this->getElementFromVector(mcs_mom, i, 0.f) - current_range_mom) / current_range_mom) < 0.5 : true));
                    mask[i] = (this->getElementFromVector(ts, i, 0.f) > 0.8f) && (this->getElementFromVector(pid, i, 0.f) > 0.2f) && fiducial && quality;
                }
                return mask;
            },
            {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v",
             "trk_start_x_v", "trk_end_x_v", "trk_start_y_v", "trk_end_y_v",
             "trk_start_z_v", "trk_end_z_v", "trk_mcs_muon_mom_v", "trk_range_muon_mom_v"}
        );

        auto df_sel_idx = df_mu_mask.Define("selected_muon_idx",
            [this](const ROOT::RVec<float>& l, const ROOT::RVec<bool>& m) {
                return this->getIndexFromVectorSort(l, m, 0, false);
            }, {"trk_len_v", "muon_candidate_selection_mask_vec"}
        );

        auto df_mu_props = df_sel_idx
            .Define("selected_muon_length", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_len_v", "selected_muon_idx"})
            .Define("selected_muon_momentum_range", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_range_muon_mom_v", "selected_muon_idx"})
            .Define("selected_muon_momentum_mcs", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_mcs_muon_mom_v", "selected_muon_idx"})
            .Define("selected_muon_phi", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -999.f); }, {"trk_phi_v", "selected_muon_idx"})
            .Define("selected_muon_cos_theta", [this](const ROOT::RVec<float>& v, int i) { float t = this->getElementFromVector(v, i, -999.f); return (std::abs(t) < 100.f && std::isfinite(t)) ? std::cos(t) : -999.f; }, {"trk_theta_v", "selected_muon_idx"})
            .Define("selected_muon_energy", [](float mom) { const float M = 0.105658f; return (mom >= 0.f && std::isfinite(mom)) ? std::sqrt(mom * mom + M * M) : -1.f; }, {"selected_muon_momentum_range"})
            .Define("selected_muon_trk_score", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -1.f); }, {"trk_score_v", "selected_muon_idx"})
            .Define("selected_muon_llr_pid_score", [this](const ROOT::RVec<float>& v, int i) { return this->getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "selected_muon_idx"});

        return df_mu_props
            .Define("n_muon_candidates", [](const ROOT::RVec<bool>& m) { return ROOT::VecOps::Sum(m); }, {"muon_candidate_selection_mask_vec"});
    }
};

}

#endif