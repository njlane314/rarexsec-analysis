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
        df = this->defineBlipFeatures(df);

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

        d = d.Define("is_reco_fv", "reco_nu_vtx_sce_x > 5.0 && reco_nu_vtx_sce_x < 251.0 && reco_nu_vtx_sce_y > -110.0 && reco_nu_vtx_sce_y < 110.0 && reco_nu_vtx_sce_z > 20.0 && reco_nu_vtx_sce_z < 986.0");

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
        
        d = d.Define("quality_selector", "is_reco_fv && nslice == 1 && selected == 1 && nu_slice_topo_score >= 0.2 && nu_slice_topo_score <= 1.0 && _opfilter_pe_beam >= 20.0 && nhits_u > 0 && nhits_v > 0 && nhits_w > 0");
        
        return d;
    }

    ROOT::RDF::RNode defineBlipFeatures(ROOT::RDF::RNode df) const {
        auto d = df;

        // --- Original blip variables ---
        if (d.HasColumn("blip_ID")) {
            d = d.Define("n_blips", [](const ROOT::RVec<int>& v){ return (int)v.size(); }, {"blip_ID"});
        }
        if (d.HasColumn("blip_Energy")) {
            d = d.Define("total_blip_energy", [](const ROOT::RVec<float>& v) { return ROOT::VecOps::Sum(v); }, {"blip_Energy"});
        }

        // --- New creative blip variables ---

        // Number of blips passing the 'isValid' flag
        if (d.HasColumn("blip_isValid")) {
            d = d.Define("n_valid_blips", [](const ROOT::RVec<bool>& v){ return (int)ROOT::VecOps::Sum(v); }, {"blip_isValid"});
        }

        // Energy of the most energetic blip in the event
        if (d.HasColumn("blip_Energy")) {
            d = d.Define("max_blip_energy", [](const ROOT::RVec<float>& v){ return v.empty() ? 0.f : ROOT::VecOps::Max(v); }, {"blip_Energy"});
        }

        // Average energy of blips in the event
        if (d.HasColumn("n_blips") && d.HasColumn("total_blip_energy")) {
            d = d.Define("avg_blip_energy", [](int n, float total_e){ return n > 0 ? total_e / (float)n : 0.f; }, {"n_blips", "total_blip_energy"});
        }

        // A vector containing the 3D distance of each blip from the reconstructed neutrino vertex
        if (d.HasColumn("blip_X") && d.HasColumn("reco_nu_vtx_sce_x")) {
            auto blip_vtx_dist_lambda = [](const ROOT::RVec<float>& bx, const ROOT::RVec<float>& by, const ROOT::RVec<float>& bz, float vx, float vy, float vz) {
                ROOT::RVec<float> dists;
                dists.reserve(bx.size());
                for (size_t i = 0; i < bx.size(); ++i) {
                    float dx = bx[i] - vx;
                    float dy = by[i] - vy;
                    float dz = bz[i] - vz;
                    dists.push_back(std::sqrt(dx*dx + dy*dy + dz*dz));
                }
                return dists;
            };
            d = d.Define("blip_dist_from_vtx", blip_vtx_dist_lambda,
                {"blip_X", "blip_Y", "blip_Z", "reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z"});
        }

         d = d.Define("mean_blip_dist_from_vtx", [](const ROOT::RVec<float>& dists){
            return dists.empty() ? -1.f : ROOT::VecOps::Mean(dists);
        }, {"blip_dist_from_vtx"});

        if (d.HasColumn("blip_dist_from_vtx") && d.HasColumn("blip_Energy")) {

            // --- Exponentially-Weighted Energy Moments for Multiple Length Scales ---
            auto exp_moments_lambda = [](const ROOT::RVec<float>& dists, const ROOT::RVec<float>& energies) -> std::vector<float> {
                const std::vector<float> mean_free_paths = {10.0f, 12.0f, 15.0f, 20.0f, 30.0f, 40.0f, 50.0f, 100.0f, 18.0f, 22.0f, 25.0f, 60.0f, 75.0f};
                std::vector<float> moments;
                moments.reserve(mean_free_paths.size());

                for (const float mfp : mean_free_paths) {
                    float moment = 0.f;
                    for (size_t i = 0; i < dists.size(); ++i) {
                        moment += energies[i] * exp(-dists[i] / mfp);
                    }
                    moments.push_back(moment);
                }
                return moments;
            };
            d = d.Define("blip_exp_energy_moments", exp_moments_lambda, {"blip_dist_from_vtx", "blip_Energy"});

            d = d.Define("blip_exp_moment_10cm", [](const std::vector<float>& v){ return v[0]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_12cm", [](const std::vector<float>& v){ return v[1]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_15cm", [](const std::vector<float>& v){ return v[2]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_20cm", [](const std::vector<float>& v){ return v[3]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_30cm", [](const std::vector<float>& v){ return v[4]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_40cm", [](const std::vector<float>& v){ return v[5]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_50cm", [](const std::vector<float>& v){ return v[6]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_100cm", [](const std::vector<float>& v){ return v[7]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_18cm", [](const std::vector<float>& v){ return v[8]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_22cm", [](const std::vector<float>& v){ return v[9]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_25cm", [](const std::vector<float>& v){ return v[10]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_60cm", [](const std::vector<float>& v){ return v[11]; }, {"blip_exp_energy_moments"});
            d = d.Define("blip_exp_moment_75cm", [](const std::vector<float>& v){ return v[12]; }, {"blip_exp_energy_moments"});
        }

        // The standard deviation of the blip distances. A smaller value might indicate
        // that blips are more tightly clustered near the vertex, as seen in your signal.
        d = d.Define("std_dev_blip_dist_from_vtx", [](const ROOT::RVec<float>& dists){
            if (dists.size() < 2) return 0.f;
            return (float)ROOT::VecOps::StdDev(dists);
        }, {"blip_dist_from_vtx"});

        d = d.Define("n_blips_halo_30_50cm", [](const ROOT::RVec<float>& dists){
            const float inner_radius = 0.0f;
            const float outer_radius = 150.0f;
            int count = 0;
            for (float d_val : dists) {
                if (d_val >= inner_radius && d_val < outer_radius) {
                    count++;
                }
            }
            return count;
        }, {"blip_dist_from_vtx"});

        // Directly test your observation at 50cm. This variable calculates the
        // fraction of an event's blips that fall within that 50cm radius.
        // This should be a powerful discriminator.
        d = d.Define("fraction_blips_under_50cm", [](const ROOT::RVec<float>& dists){
            if (dists.empty()) return 0.f;
            int near_count = 0;
            for (float d_val : dists) {
                if (d_val < 50.0f) {
                    near_count++;
                }
            }
            return (float)near_count / dists.size();
        }, {"blip_dist_from_vtx"});

        if (d.HasColumn("blip_dist_from_vtx")) {
            const float proximity_radius_cm = 100.0f;

            // Count how many blips are inside the defined radius
            d = d.Define("n_blips_near_vtx", [proximity_radius_cm](const ROOT::RVec<float>& dists){
                int count = 0;
                for (float d : dists) {
                    if (d < proximity_radius_cm) {
                        count++;
                    }
                }
                return count;
            }, {"blip_dist_from_vtx"});

            // Also, calculate the fraction of the total blip energy that comes
            // from these "near-vertex" blips. This might be more robust than just a count.
            if (d.HasColumn("blip_Energy") && d.HasColumn("total_blip_energy")) {
                auto nearby_energy_lambda = [proximity_radius_cm](const ROOT::RVec<float>& dists, const ROOT::RVec<float>& energies, float total_blip_E) -> float {
                    if (total_blip_E < 1e-6) return 0.f;
                    float nearby_energy = 0.f;
                    for (size_t i=0; i < dists.size(); ++i) {
                        if (dists[i] < proximity_radius_cm) {
                            nearby_energy += energies[i];
                        }
                    }
                    return nearby_energy / total_blip_E;
                };
                d = d.Define("nearby_blip_energy_fraction", nearby_energy_lambda, {"blip_dist_from_vtx", "blip_Energy", "total_blip_energy"});
            }
        }
        
        // Number of blips that are very close to a reconstructed track (< 5 cm)
        if (d.HasColumn("blip_ProxTrkDist")) {
            d = d.Define("n_blips_prox_track", [](const ROOT::RVec<float>& dists){
                int count = 0;
                for (float dist : dists) {
                    if (dist < 5.0 && dist >= 0) {
                        count++;
                    }
                }
                return count;
            }, {"blip_ProxTrkDist"});
        }
        
        // --- Truth-level variables (for MC studies) ---
        
        // Number of blips whose leading particle was a neutron
        if (d.HasColumn("blip_pdg")) {
            d = d.Define("n_neutron_blips_truth", [](const ROOT::RVec<int>& pdgs){
                int count = 0;
                for (int pdg : pdgs) {
                    if (pdg == 2112) {
                        count++;
                    }
                }
                return count;
            }, {"blip_pdg"});
        }

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

        auto avg_dedx_calculator = [](
            const ROOT::RVec<float>& u_dedx,
            const ROOT::RVec<float>& v_dedx,
            const ROOT::RVec<float>& y_dedx) {

            size_t n_tracks = u_dedx.size();
            ROOT::RVec<float> avg_dedx(n_tracks);

            for (size_t i = 0; i < n_tracks; ++i) {
                float sum = 0.0;
                int count = 0;
                if (u_dedx[i] > 0) { sum += u_dedx[i]; count++; }
                if (v_dedx[i] > 0) { sum += v_dedx[i]; count++; }
                if (y_dedx[i] > 0) { sum += y_dedx[i]; count++; }

                if (count > 0) {
                    avg_dedx[i] = sum / count;
                } else {
                    avg_dedx[i] = -1.0;
                }
            }
            return avg_dedx;
        };

        if (d.HasColumn("trk_trunk_rr_dEdx_u_v") && d.HasColumn("trk_trunk_rr_dEdx_v_v") && d.HasColumn("trk_trunk_rr_dEdx_y_v")) {
             d = d.Define("trk_trunk_rr_dEdx_avg_v", avg_dedx_calculator, {"trk_trunk_rr_dEdx_u_v", "trk_trunk_rr_dEdx_v_v", "trk_trunk_rr_dEdx_y_v"});
        
            d = d.Define("muon_candidate_mask",
                [this](const ROOT::RVec<float> ts, const ROOT::RVec<float> pid,
                    const ROOT::RVec<float> l, const ROOT::RVec<float> d,
                    const ROOT::RVec<unsigned int> generations, const ROOT::RVec<float> trunk_rr_dEdx_avg) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool quality = (this->getElementFromVector(ts, i, 0.f) >= 0.3f && this->getElementFromVector(ts, i, 0.f) <= 1.0f &&
                                    this->getElementFromVector(pid, i, 0.f) >= -0.2f && this->getElementFromVector(pid, i, 0.f) <= 1.0f &&
                                    this->getElementFromVector(l, i, 0.f) >= 5.0f && this->getElementFromVector(trunk_rr_dEdx_avg, i, 0.f) <= 3.0f);
                    mask[i] = quality;
                }
                return mask;
            }, {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "pfp_generation_v", "trk_trunk_rr_dEdx_avg_v"});

           d = d.Define("proton_candidate_mask",
                [this](const ROOT::RVec<float> ts, const ROOT::RVec<float> pid,
                    const ROOT::RVec<float> l, const ROOT::RVec<float> d,
                    const ROOT::RVec<unsigned int> generations, const ROOT::RVec<bool>& muon_mask) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool quality = (this->getElementFromVector(ts, i, 0.f) > 0.7f &&
                                    this->getElementFromVector(pid, i, 0.f) < 0.4f &&
                                    this->getElementFromVector(l, i, 0.f) > 10.0f &&
                                    this->getElementFromVector(d, i, 0.f) < 2.0f &&
                                    this->getElementFromVector(generations, i, 0) == 2 &&
                                    !muon_mask[i]);
                    mask[i] = quality;
                }
                return mask;
            }, {"trk_score_v", "trk_llr_pid_score_v", "trk_len_v", "trk_distance_v", "pfp_generation_v", "muon_candidate_mask"});

            d = d.Define("n_muons", 
                [](const ROOT::RVec<bool>& muon_mask) {
                return ROOT::VecOps::Sum(muon_mask);
            }, {"muon_candidate_mask"});

            d = d.Define("muon_candidate_selector", "n_muons > 0");
            
            d = d.Define("n_protons", 
                [](const ROOT::RVec<bool>& proton_mask) {
                    return ROOT::VecOps::Sum(proton_mask);
            }, {"proton_candidate_mask"});
        }

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