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
#include <iostream>

#include "TVector3.h"
#include "TLorentzVector.h"

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
        df = this->defineEventFeatures(df);
        df = this->defineTopologicalFeatures(df);

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

    inline double PionMomentum(double TrackLength) const { return 0.25798 + 0.0024088*TrackLength - 0.18828*pow(TrackLength,-0.11687); }
    inline double ProtonMomentum(double TrackLength) const { return 14.96 + 0.0043489*TrackLength - 14.688*pow(TrackLength,-0.0053518); }
    inline double getKE(double momentum, double mass) const { return sqrt(momentum*momentum + mass*mass) - mass; }

    double angleBetweenTracks(
        const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z,
        int idx1, int idx2
    ) const {
        if (idx1 < 0 || idx2 < 0 || idx1 == idx2) return -1.0;
        TVector3 vec1(this->getElementFromVector(dir_x, idx1, 0.f), this->getElementFromVector(dir_y, idx1, 0.f), this->getElementFromVector(dir_z, idx1, 0.f));
        TVector3 vec2(this->getElementFromVector(dir_x, idx2, 0.f), this->getElementFromVector(dir_y, idx2, 0.f), this->getElementFromVector(dir_z, idx2, 0.f));
        if (vec1.Mag() == 0 || vec2.Mag() == 0) return -1.0;
        return vec1.Angle(vec2);
    }

    TLorentzVector getTrackLorentzVector(int idx, const ROOT::RVec<float>& energy, const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z, double mass) const {
        TLorentzVector p(0,0,0,0);
        if (idx < 0) return p;
        float E = this->getElementFromVector(energy, idx, 0.f);
        if (E <= 0 || E*E < mass*mass) return p;
        double momentum = sqrt(E*E - mass*mass);
        p.SetPxPyPzE(momentum * this->getElementFromVector(dir_x, idx, 0.f),
                     momentum * this->getElementFromVector(dir_y, idx, 0.f),
                     momentum * this->getElementFromVector(dir_z, idx, 0.f),
                     E);
        return p;
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
                                               "true_nu_vtx_x", "nu_pdg", "ccnc", "interaction", "mcf_npp", "mcf_npm", "mcf_np0", "mcf_npr"};
        bool has_all_truth_cols = is_mc;
        if (is_mc) {
            for(const auto& col : truth_cols){ if(!df.HasColumn(col)){ has_all_truth_cols = false; break; } }
        }

        if (is_mc && has_all_truth_cols) {
            df_with_defs = df_with_defs.Define("in_fv", "(true_nu_vtx_x > 5.0 && true_nu_vtx_x < 251.0 && true_nu_vtx_y > -110.0 && true_nu_vtx_y < 110.0 && true_nu_vtx_z > 20.0 && true_nu_vtx_z < 986.0)");
            df_with_defs = df_with_defs.Define("mcf_strangeness", "mcf_nkp + mcf_nkm + mcf_nk0 + mcf_nlambda + mcf_nsigma_p + mcf_nsigma_0 + mcf_nsigma_m");
            df_with_defs = df_with_defs.Define("mc_n_pions", "mcf_npp + mcf_npm + mcf_np0");
            df_with_defs = df_with_defs.Define("mc_n_protons", "mcf_npr");

            df_with_defs = df_with_defs.Define("inclusive_strange_channels",
                [](bool is_fid, int nu, int cc, int strange, int n_pi, int n_p) {
                    if (!is_fid) return 98;
                    if (cc == 1) return 31;
                    if (std::abs(nu) == 12 && cc == 0) return 30;

                    if (std::abs(nu) == 14 && cc == 0) {  
                        if (strange == 1) return 10;  
                        if (strange > 1) return 11;  

                        if (n_p >= 1 && n_pi == 0) return 20;
                        if (n_p == 0 && n_pi >= 1) return 21;  
                        if (n_p >= 1 && n_pi >= 1) return 22;  
                        
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
                        if (strange == 0) return 32;
                        if ((n_kp == 1 || n_km == 1) && strange == 1) return 50; 
                        if (n_k0 == 1 && strange == 1) return 51; 
                        if (n_lambda == 1 && strange == 1) return 52; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && strange == 1) return 53; 
                        
                        if (n_lambda == 1 && (n_kp == 1 || n_km == 1) && strange == 2) return 54; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && n_k0 == 1 && strange == 2) return 55; 
                        if ((n_sigma_p == 1 || n_sigma_m == 1) && (n_kp == 1 || n_km == 1) && strange == 2) return 56;
                        if (n_lambda == 1 && n_k0 == 1 && strange == 2) return 57; 
                        if (n_kp == 1 && n_km == 1 && strange == 2) return 58; 
                        if (n_sigma_0 == 1 && strange == 1) return 59;
                        if (n_sigma_0 == 1 && n_kp == 1 && strange == 2) return 60;

                        return 61;
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

    ROOT::RDF::RNode defineEventFeatures(ROOT::RDF::RNode df) const {
        auto d = df;

        d = d.Define("nu_slice_topo_score",
            [this](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                return this->getElementFromVector(all_slice_scores, static_cast<int>(neutrino_slice_id), -999.f);
            }, {"slice_topo_score_v", "slice_id"}
        );

        d = d.Define("n_pfp_gen_2",
            [](const ROOT::RVec<unsigned int>& generations) {
                return ROOT::VecOps::Sum(generations == 2);
            }, {"pfp_generation_v"});

        d = d.Define("n_pfp_gen_3",
            [](const ROOT::RVec<unsigned int>& generations) {
                return ROOT::VecOps::Sum(generations == 3);
            }, {"pfp_generation_v"});
        
        return d;
    }

    ROOT::RDF::RNode defineTopologicalFeatures(ROOT::RDF::RNode df) const {
        auto d = df;
        
        std::vector<std::string> essential_cols = {"trk_len_v", "pfp_generation_v", "shr_energy_y_v", "trk_distance_v", "trk_score_v", "trk_llr_pid_score_v", "trk_pfp_id_v", "shr_pfp_id_v"};
        for(const auto& col : essential_cols){
            if(!d.HasColumn(col)){
                std::cerr << "Warning: Essential column '" << col << "' not found. Skipping topological feature definitions." << std::endl;
                return d;
            }
        }

        d = d.Define("is_reco_fv", "reco_nu_vtx_sce_x > 5.0 && reco_nu_vtx_sce_x < 251.0 && reco_nu_vtx_sce_y > -110.0 && reco_nu_vtx_sce_y < 110.0 && reco_nu_vtx_sce_z > 20.0 && reco_nu_vtx_sce_z < 986.0");
        
        auto get_track_is_primary = [](const ROOT::RVec<unsigned int>& pfp_gen, const ROOT::RVec<unsigned long>& trk_pfp_id, const ROOT::RVec<float>& dist) {
            ROOT::RVec<bool> is_primary;
            is_primary.reserve(trk_pfp_id.size());
            for (size_t i = 0; i < trk_pfp_id.size(); ++i) {
                unsigned long id = trk_pfp_id[i];
                bool primary = false;
                if (id < pfp_gen.size()) {
                    primary = (pfp_gen[id] == 2);
                }
                is_primary.push_back(primary && (dist[i] < 4.0));
            }
            return is_primary;
        };
        d = d.Define("track_is_primary", get_track_is_primary, {"pfp_generation_v", "trk_pfp_id_v", "trk_distance_v"});

        auto get_shower_is_primary = [](const ROOT::RVec<unsigned int>& pfp_gen, const ROOT::RVec<unsigned long>& shr_pfp_id) {
            ROOT::RVec<bool> is_primary;
            is_primary.reserve(shr_pfp_id.size());
            for (unsigned long id : shr_pfp_id) {
                if (id < pfp_gen.size()) {
                    is_primary.push_back(pfp_gen[id] == 2);
                } else {
                    is_primary.push_back(false);
                }
            }
            return is_primary;
        };
        d = d.Define("shower_is_primary", get_shower_is_primary, {"pfp_generation_v", "shr_pfp_id_v"});


        auto track_good_quality_builder = [](const ROOT::RVec<float>& len, const ROOT::RVec<float>& score) {
            if (len.size() != score.size()) return ROOT::RVec<bool>(len.size(), false);
            ROOT::RVec<bool> mask(len.size());
            for(size_t i = 0; i < len.size(); ++i) {
                mask[i] = (len[i] > 10.0) && (score[i] > 0.5);
            }
            return mask;
        };
        d = d.Define("track_is_good_quality", track_good_quality_builder, {"trk_len_v", "trk_score_v"});
        
        d = d.Define("shower_is_good_quality", [](const ROOT::RVec<float>& energy) { return ROOT::RVec<bool>(energy > 0.07); }, {"shr_energy_y_v"});

        auto muon_mask_builder = [](const ROOT::RVec<bool>& is_primary, const ROOT::RVec<bool>& is_good, const ROOT::RVec<float>& pid_score, const ROOT::RVec<float>& trk_score) {
            size_t n = is_primary.size();
            if (is_good.size() != n || pid_score.size() != n || trk_score.size() != n) return ROOT::RVec<bool>(n, false);
            ROOT::RVec<bool> mask(n);
            for (size_t i = 0; i < n; ++i) {
                mask[i] = is_primary[i] && is_good[i] && pid_score[i] > 0.6 && trk_score[i] > 0.9;
            }
            return mask;
        };
        d = d.Define("muon_candidate_mask", muon_mask_builder, {"track_is_primary", "track_is_good_quality", "trk_llr_pid_score_v", "trk_score_v"});

        auto proton_mask_builder = [](const ROOT::RVec<bool>& is_primary, const ROOT::RVec<bool>& is_good, const ROOT::RVec<float>& pid_score, const ROOT::RVec<bool>& muon_mask) {
            size_t n = is_primary.size();
            if (is_good.size() != n || pid_score.size() != n || muon_mask.size() != n) return ROOT::RVec<bool>(n, false);
            ROOT::RVec<bool> mask(n);
            for (size_t i = 0; i < n; ++i) {
                mask[i] = is_primary[i] && is_good[i] && pid_score[i] < -0.4 && !muon_mask[i];
            }
            return mask;
        };
        d = d.Define("proton_candidate_mask", proton_mask_builder, {"track_is_primary", "track_is_good_quality", "trk_llr_pid_score_v", "muon_candidate_mask"});

        auto pion_mask_builder = [](const ROOT::RVec<bool>& is_primary, const ROOT::RVec<bool>& is_good, const ROOT::RVec<float>& pid_score, const ROOT::RVec<bool>& muon_mask, const ROOT::RVec<bool>& proton_mask) {
            size_t n = is_primary.size();
            if (is_good.size() != n || pid_score.size() != n || muon_mask.size() != n || proton_mask.size() != n) return ROOT::RVec<bool>(n, false);
            ROOT::RVec<bool> mask(n);
            for (size_t i = 0; i < n; ++i) {
                mask[i] = is_primary[i] && is_good[i] && pid_score[i] > 0.4 && !muon_mask[i] && !proton_mask[i];
            }
            return mask;
        };
        d = d.Define("pion_candidate_mask", pion_mask_builder, {"track_is_primary", "track_is_good_quality", "trk_llr_pid_score_v", "muon_candidate_mask", "proton_candidate_mask"});

        auto electron_mask_builder = [](const ROOT::RVec<bool>& is_primary, const ROOT::RVec<bool>& is_good, const ROOT::RVec<float>& dedx, const ROOT::RVec<float>& moliere) {
            size_t n = is_primary.size();
            if (is_good.size() != n || dedx.size() != n || moliere.size() != n) return ROOT::RVec<bool>(n, false);
            ROOT::RVec<bool> mask(n);
            for (size_t i = 0; i < n; ++i) {
                mask[i] = is_primary[i] && is_good[i] && dedx[i] > 0.5 && dedx[i] < 5.5 && moliere[i] < 9.0;
            }
            return mask;
        };
        d = d.Define("electron_candidate_mask", electron_mask_builder, {"shower_is_primary", "shower_is_good_quality", "shr_tkfit_dedx_y_v", "shr_moliere_avg_v"});

        d = d.Define("leading_muon_idx", "static_cast<int>(ROOT::VecOps::ArgMax(trk_len_v * muon_candidate_mask))");
        d = d.Define("leading_proton_idx", "static_cast<int>(ROOT::VecOps::ArgMax(trk_len_v * proton_candidate_mask))");
        d = d.Define("leading_pion_idx", "static_cast<int>(ROOT::VecOps::ArgMax(trk_len_v * pion_candidate_mask))");
        d = d.Define("leading_electron_idx", "static_cast<int>(ROOT::VecOps::ArgMax(shr_energy_y_v * electron_candidate_mask))");

        d = d.Define("n_muons", "static_cast<int>(ROOT::VecOps::Sum(muon_candidate_mask))");
        d = d.Define("n_protons", "static_cast<int>(ROOT::VecOps::Sum(proton_candidate_mask))");
        d = d.Define("n_pions", "static_cast<int>(ROOT::VecOps::Sum(pion_candidate_mask))");
        d = d.Define("n_electrons", "static_cast<int>(ROOT::VecOps::Sum(electron_candidate_mask))");
        
        d = d.Define("muon_len", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_len_v", "leading_muon_idx"})
                .Define("muon_pid", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "leading_muon_idx"})
                .Define("muon_pida", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_pida_v", "leading_muon_idx"})
                .Define("muon_chi2_muon", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_pid_chimu_v", "leading_muon_idx"})
                .Define("muon_chi2_proton", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_pid_chipr_v", "leading_muon_idx"})
                .Define("muon_mcs_mom", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_mcs_muon_mom_v", "leading_muon_idx"})
                .Define("muon_deflection_std", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_avg_deflection_stdev_v", "leading_muon_idx"})
                .Define("muon_end_sp", [this](const ROOT::RVec<int>& v, int i){ return static_cast<unsigned int>(this->getElementFromVector(v, i, 0u)); }, {"trk_end_spacepoints_v", "leading_muon_idx"});

        d = d.Define("proton_len", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_len_v", "leading_proton_idx"})
                .Define("proton_pid", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -999.f); }, {"trk_llr_pid_score_v", "leading_proton_idx"})
                .Define("proton_pida", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_pida_v", "leading_proton_idx"})
                .Define("proton_chi2_proton", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_pid_chipr_v", "leading_proton_idx"})
                .Define("proton_E", [this](const ROOT::RVec<float>& v, int i){ return this->getElementFromVector(v, i, -1.f); }, {"trk_energy_proton_v", "leading_proton_idx"});

        auto p4_def = [this](int idx, const ROOT::RVec<float>& energy, const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z, double mass) {
            return this->getTrackLorentzVector(idx, energy, dir_x, dir_y, dir_z, mass);
        };
        
        d = d.Define("muon_p4", 
            [this](int idx, const ROOT::RVec<float>& energy, const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z) {
                const double muon_mass = 0.105658; 
                return this->getTrackLorentzVector(idx, energy, dir_x, dir_y, dir_z, muon_mass);
            }, 
            {"leading_muon_idx", "trk_energy_muon_v", "trk_dir_x_v", "trk_dir_y_v", "trk_dir_z_v"});

        d = d.Define("proton_p4", 
            [this](int idx, const ROOT::RVec<float>& energy, const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z) {
                const double proton_mass = 0.938272; 
                return this->getTrackLorentzVector(idx, energy, dir_x, dir_y, dir_z, proton_mass);
            },
            {"leading_proton_idx", "trk_energy_proton_v", "trk_dir_x_v", "trk_dir_y_v", "trk_dir_z_v"});

        auto buildHadronicSystem = [this](const ROOT::RVec<bool>& p_mask, const ROOT::RVec<bool>& pi_mask, const ROOT::RVec<float>& len, const ROOT::RVec<float>& E_p, const ROOT::RVec<float>& dx, const ROOT::RVec<float>& dy, const ROOT::RVec<float>& dz) {
            TLorentzVector p4_hadronic(0,0,0,0);
            for(size_t i=0; i<len.size(); ++i){
                if(i < p_mask.size() && p_mask[i]) {
                    p4_hadronic += this->getTrackLorentzVector(i, E_p, dx, dy, dz, 0.938272);
                }
                if(i < pi_mask.size() && pi_mask[i]) {
                    double pi_mom = this->PionMomentum(this->getElementFromVector(len, i, 0.f));
                    double E = sqrt(pi_mom*pi_mom + 0.13957*0.13957);
                    TLorentzVector p_pi;
                    if (E*E >= 0.13957*0.13957) {
                        double momentum = sqrt(E*E - 0.13957*0.13957);
                        p_pi.SetPxPyPzE(momentum * this->getElementFromVector(dx, i, 0.f),
                                            momentum * this->getElementFromVector(dy, i, 0.f),
                                            momentum * this->getElementFromVector(dz, i, 0.f),
                                            E);
                        p4_hadronic += p_pi;
                    }
                }
            }
            return p4_hadronic;
        };
        d = d.Define("hadronic_p4", buildHadronicSystem, {"proton_candidate_mask", "pion_candidate_mask", "trk_len_v", "trk_energy_proton_v", "trk_dir_x_v", "trk_dir_y_v", "trk_dir_z_v"});
        
        d = d.Define("hadronic_E", "hadronic_p4.E()");
        d = d.Define("hadronic_W", "hadronic_p4.M() > 0 ? hadronic_p4.M() : 0.0");
        
        d = d.Define("total_E_visible_numu", "muon_p4.E() + hadronic_E");
        d = d.Define("missing_pT_numu", "(muon_p4 + hadronic_p4).Pt()");
        d = d.Define("muon_E_fraction", "total_E_visible_numu > 0 ? muon_p4.E() / total_E_visible_numu : -1.0");
        
        auto angle_def = [this](const ROOT::RVec<float>& dir_x, const ROOT::RVec<float>& dir_y, const ROOT::RVec<float>& dir_z, int idx1, int idx2) {
            return this->angleBetweenTracks(dir_x, dir_y, dir_z, idx1, idx2);
        };
        d = d.Define("muon_proton_angle", angle_def, {"trk_dir_x_v", "trk_dir_y_v", "trk_dir_z_v", "leading_muon_idx", "leading_proton_idx"});

        d = d.Define("is_1mu0p0pi", "n_muons == 1 && n_protons == 0 && n_pions == 0 && n_pfp_gen_2 == 1");
        d = d.Define("is_1mu1p0pi", "n_muons == 1 && n_protons == 1 && n_pions == 0 && n_pfp_gen_2 == 2");
        d = d.Define("is_1muNp0pi", "n_muons == 1 && n_protons > 1 && n_pions == 0 && n_pfp_gen_2 > 2");
        d = d.Define("is_1mu0p1pi", "n_muons == 1 && n_protons == 0 && n_pions == 1 && n_pfp_gen_2 == 2");
        d = d.Define("is_1mu1p1pi", "n_muons == 1 && n_protons == 1 && n_pions == 1 && n_pfp_gen_2 == 3");
        d = d.Define("is_1mu_other", "n_muons == 1 && !(is_1mu0p0pi || is_1mu1p0pi || is_1muNp0pi || is_1mu0p1pi || is_1mu1p1pi)");
        
        d = d.Define("is_1e0p", "n_electrons == 1 && n_protons == 0 && n_muons == 0 && n_pfp_gen_2 == 1");
        d = d.Define("is_1e1p", "n_electrons == 1 && n_protons == 1 && n_muons == 0 && n_pfp_gen_2 == 2");
        d = d.Define("is_1eNp", "n_electrons == 1 && n_protons > 0 && n_muons == 0 && n_pfp_gen_2 > 2");
        
        d = d.Define("is_mulit_mu", "n_muons > 1 ");
        d = d.Define("is_nc_like", "n_muons == 0 && n_electrons == 0");
        d = d.Define("is_cc_nue_like", "n_electrons == 1 && n_muons == 0 && n_pfp_gen_2 > 1");

        if (d.HasColumn("trk_nhits_u_v"))
            d = d.Define("trk_nhits_u_v_float", "static_cast<ROOT::RVec<float>>(trk_nhits_u_v)");
        if (d.HasColumn("trk_nhits_v_v"))
            d = d.Define("trk_nhits_v_v_float", "static_cast<ROOT::RVec<float>>(trk_nhits_v_v)");
        if (d.HasColumn("trk_nhits_y_v"))
            d = d.Define("trk_nhits_y_v_float", "static_cast<ROOT::RVec<float>>(trk_nhits_y_v)");
        if (d.HasColumn("trk_end_spacepoints_v"))
            d = d.Define("trk_end_spacepoints_v_float", "static_cast<ROOT::RVec<float>>(trk_end_spacepoints_v)");

        return d;
    }
};

}

#endif