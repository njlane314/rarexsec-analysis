#ifndef DATALOADERHELPERS_H
#define DATALOADERHELPERS_H

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"
#include <vector>
#include <string>
#include <cmath>     
#include <algorithm> 
#include <utility>  

#include "SampleTypes.h"

namespace AnalysisFramework {

template<typename T_vec, typename T_val = typename T_vec::value_type>
T_val GetElementFromVector(const T_vec& vec, int index, T_val default_val = T_val{}) {
    if (index >= 0 && static_cast<size_t>(index) < vec.size()) {
        return vec[index];
    }
    return default_val;
}

inline int GetIndexFromVectorSort(const ROOT::RVec<float>& values_vec, const ROOT::RVec<bool>& mask_vec, int n_th_idx = 0, bool asc = false) {
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

ROOT::RDF::RNode AddEventCategories(ROOT::RDF::RNode df, const SampleType& sample_type) {
    auto df_with_defs = df;
    bool is_mc = isSampleMC(sample_type);

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
            if (isSampleData(sample_type)) {
                cat = 0;
            } else if (isSampleEXT(sample_type)) {
                cat = 1;
            } else if (isSampleDirt(sample_type)) {
                cat = 2;
            } else if (isSampleMC(sample_type)) {
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
                        } else { // NC
                            if (npi_char_true == 0) { if (npr_true == 0) cat = 110; else if (npr_true == 1) cat = 111; else cat = 112; }
                            else if (npi_char_true == 1) { if (npr_true == 0) cat = 113; else if (npr_true == 1) cat = 114; else cat = 115; }
                            else cat = 116;
                        }
                    } else if (isnue) {
                        if (iscc) {
                            if (npi_char_true == 0) { if (npr_true == 0) cat = 200; else if (npr_true == 1) cat = 201; else cat = 202; }
                            else if (npi_char_true == 1) { if (npr_true == 0) cat = 203; else if (npr_true == 1) cat = 204; else cat = 205; }
                            else cat = 206;
                        } else cat = 210; // NC
                    } else cat = 998; // Other neutrino
                }
            }
            return cat;
        }, {"nu_pdg", "nu_ccnc", "mc_n_charged_pions_true", "mc_n_protons_true", "inclusive_strangeness_multiplicity_type"}
    );

    return df_with_event_category.Alias("event_category", "event_category_val");
}

ROOT::RDF::RNode ProcessNuMuVariables(ROOT::RDF::RNode df, SampleType sample_type) {
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
            return GetIndexFromVectorSort(l, m, 0, false);
        }, {"trk_len_v", "muon_candidate_selection_mask_vec"}
    );

    auto df_mu_props = df_sel_idx
        .Define("selected_muon_length", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -1.f); }, {"trk_len_v", "selected_muon_idx"})
        .Define("selected_muon_momentum_range", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -1.f); }, {"trk_range_muon_mom_v", "selected_muon_idx"})
        .Define("selected_muon_momentum_mcs", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -1.f); }, {"trk_mcs_muon_mom_v", "selected_muon_idx"})
        .Define("selected_muon_phi", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -999.f); }, {"trk_phi_v", "selected_muon_idx"})
        .Define("selected_muon_cos_theta", [](const ROOT::RVec<float>& v, int i) { float t = GetElementFromVector(v, i, -999.f); return (t > -998.f && std::isfinite(t)) ? std::cos(t) : -999.f; }, {"trk_theta_v", "selected_muon_idx"})
        .Define("selected_muon_energy", [](float mom) { const float M = 0.105658f; return (mom >= 0.f && std::isfinite(mom)) ? std::sqrt(mom * mom + M * M) : -1.f; }, {"selected_muon_momentum_range"})
        .Define("selected_muon_trk_score", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -1.f); }, {"trk_score_v", "selected_muon_idx"})
        .Define("selected_muon_llr_pid_score", [](const ROOT::RVec<float>& v, int i) { return GetElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "selected_muon_idx"});

    return df_mu_props
        .Define("n_muon_candidates", [](const ROOT::RVec<bool>& m) { return ROOT::VecOps::Sum(m); }, {"muon_candidate_selection_mask_vec"});
}

}

#endif // DATALOADERHELPERS_H