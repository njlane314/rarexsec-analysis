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
        if (variable_options.load_blips) {
            df = this->defineBlipFeatures(df);
        }

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

        if (is_mc) {
            df_with_defs = df_with_defs.Define("in_fv", "(neutrino_vertex_x > 5.0 && neutrino_vertex_x < 251.0 && neutrino_vertex_y > -110.0 && neutrino_vertex_y < 110.0 && neutrino_vertex_z > 20.0 && neutrino_vertex_z < 986.0)");
            df_with_defs = df_with_defs.Define("mc_n_strangeness", "count_kaon_plus + count_kaon_minus + count_kaon_zero + count_lambda + count_sigma_plus + count_sigma_zero + count_sigma_minus");
            df_with_defs = df_with_defs.Define("mc_n_pions", "count_pi_plus + count_pi_minus");
            df_with_defs = df_with_defs.Define("mc_n_protons", "count_proton");

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
                }, {"in_fv", "neutrino_pdg", "interaction_ccnc", "mc_n_strangeness", "mc_n_pions", "mc_n_protons"});

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
                }, {"in_fv", "neutrino_pdg", "interaction_ccnc", "mc_n_strangeness", "count_kaon_plus", "count_kaon_minus", "count_kaon_zero", "count_lambda", "count_sigma_plus", "count_sigma_zero", "count_sigma_minus"});
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

        d = d.Define("is_reco_fv", "reco_neutrino_vertex_sce_x > 5.0 && reco_neutrino_vertex_sce_x < 251.0 && reco_neutrino_vertex_sce_y > -110.0 && reco_neutrino_vertex_sce_y < 110.0 && reco_neutrino_vertex_sce_z > 20.0 && reco_neutrino_vertex_sce_z < 986.0");

        d = d.Define("nu_slice_topo_score",
            [this](const ROOT::RVec<float>& all_slice_scores, unsigned int neutrino_slice_id) {
                return this->getElementFromVector(all_slice_scores, static_cast<int>(neutrino_slice_id), -999.f);
            }, {"slice_topological_scores", "slice_id"}
        );

        d = d.Define("n_pfp_gen_2",
            [](const ROOT::RVec<unsigned int>& generations) {
                return ROOT::VecOps::Sum(generations == 2);
            }, {"pfp_generations"});

        d = d.Define("n_pfp_gen_3",
            [](const ROOT::RVec<unsigned int>& generations) {
                return ROOT::VecOps::Sum(generations == 3);
            }, {"pfp_generations"});
        
        d = d.Define("quality_selector", "is_reco_fv && num_slices == 1 && selection_pass == 1 && optical_filter_pe_beam >= 20.0 && total_hits_U > 0 && total_hits_V > 0 && total_hits_Y > 0");
        
        return d;
    }

    ROOT::RDF::RNode defineBlipFeatures(ROOT::RDF::RNode df) const {
        auto d = df;

        if (d.HasColumn("blip_ID")) {
            d = d.Define("n_blips", [](const ROOT::RVec<int>& v){ return (int)v.size(); }, {"blip_ID"});
        }
        if (d.HasColumn("blip_Energy")) {
            d = d.Define("total_blip_energy", [](const ROOT::RVec<float>& v) { return ROOT::VecOps::Sum(v); }, {"blip_Energy"});
        }

        if (d.HasColumn("blip_isValid")) {
            d = d.Define("n_valid_blips", [](const ROOT::RVec<bool>& v){ return (int)ROOT::VecOps::Sum(v); }, {"blip_isValid"});
        }

        if (d.HasColumn("blip_Energy")) {
            d = d.Define("max_blip_energy", [](const ROOT::RVec<float>& v){ return v.empty() ? 0.f : ROOT::VecOps::Max(v); }, {"blip_Energy"});
        }

        if (d.HasColumn("n_blips") && d.HasColumn("total_blip_energy")) {
            d = d.Define("avg_blip_energy", [](int n, float total_e){ return n > 0 ? total_e / (float)n : 0.f; }, {"n_blips", "total_blip_energy"});
        }

        if (d.HasColumn("blip_X") && d.HasColumn("reco_neutrino_vertex_sce_x")) {
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
                {"blip_X", "blip_Y", "blip_Z", "reco_neutrino_vertex_sce_x", "reco_neutrino_vertex_sce_y", "reco_neutrino_vertex_sce_z"});
        }

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

        if (d.HasColumn("track_trunk_rr_dedx_u") && d.HasColumn("track_trunk_rr_dedx_v") && d.HasColumn("track_trunk_rr_dedx_y")) {
             d = d.Define("trk_trunk_rr_dEdx_avg_v", avg_dedx_calculator, {"track_trunk_rr_dedx_u", "track_trunk_rr_dedx_v", "track_trunk_rr_dedx_y"});
        
            d = d.Define("muon_candidate_mask",
                [this](const ROOT::RVec<float> ts,
                    const ROOT::RVec<float> l, const ROOT::RVec<float> d,
                    const ROOT::RVec<unsigned int> generations, const ROOT::RVec<float> trunk_rr_dEdx_avg) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool quality = (this->getElementFromVector(ts, i, 0.f) >= 0.3f && this->getElementFromVector(ts, i, 0.f) <= 1.0f &&
                                    this->getElementFromVector(l, i, 0.f) >= 5.0f && this->getElementFromVector(trunk_rr_dEdx_avg, i, 0.f) <= 3.0f);
                    mask[i] = quality;
                }
                return mask;
            }, {"track_shower_scores", "track_length", "track_distance_to_vertex", "pfp_generations", "trk_trunk_rr_dEdx_avg_v"});

           d = d.Define("proton_candidate_mask",
                [this](const ROOT::RVec<float> ts,
                    const ROOT::RVec<float> l, const ROOT::RVec<float> d,
                    const ROOT::RVec<unsigned int> generations, const ROOT::RVec<bool>& muon_mask) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool quality = (this->getElementFromVector(ts, i, 0.f) > 0.7f &&
                                    this->getElementFromVector(l, i, 0.f) > 10.0f &&
                                    this->getElementFromVector(d, i, 0.f) < 2.0f &&
                                    this->getElementFromVector(generations, i, 0) == 2 &&
                                    !muon_mask[i]);
                    mask[i] = quality;
                }
                return mask;
            }, {"track_shower_scores", "track_length", "track_distance_to_vertex", "pfp_generations", "muon_candidate_mask"});

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

        if (d.HasColumn("track_nhits_u"))
            d = d.Define("trk_nhits_u_v_float", "static_cast<ROOT::RVec<float>>(track_nhits_u)");
        if (d.HasColumn("track_nhits_v"))
            d = d.Define("trk_nhits_v_v_float", "static_cast<ROOT::RVec<float>>(track_nhits_v)");
        if (d.HasColumn("track_nhits_y"))
            d = d.Define("trk_nhits_y_v_float", "static_cast<ROOT::RVec<float>>(track_nhits_y)");
        if (d.HasColumn("track_end_spacepoints"))
            d = d.Define("trk_end_spacepoints_v_float", "static_cast<ROOT::RVec<float>>(track_end_spacepoints)");

        if (d.HasColumn("total_hits_Y"))
            d = d.Define("evt_nhits_w_float", "static_cast<float>(total_hits_Y)");
        return d;
    }
};

}

#endif