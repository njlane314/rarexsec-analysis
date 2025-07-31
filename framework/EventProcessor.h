#ifndef EVENT_PROCESSOR_H
#define EVENT_PROCESSOR_H

#include "ProcessingStep.h"
#include "ROOT/RVec.hxx"
#include <vector>
#include <cmath>
#include <utility>
#include <algorithm>

namespace AnalysisFramework {

class EventProcessor : public ProcessingStep {
protected:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df) const override 
        auto d = df.Define("is_reco_fv", "reco_neutrino_vertex_sce_x > 5.0 && reco_neutrino_vertex_sce_x < 251.0 && reco_neutrino_vertex_sce_y > -110.0 && reco_neutrino_vertex_sce_y < 110.0 && reco_neutrino_vertex_sce_z > 20.0 && reco_neutrino_vertex_sce_z < 986.0");
        d = d.Define("n_pfp_gen_2", [](const ROOT::RVec<unsigned int>& generations) { return ROOT::VecOps::Sum(generations == 2); }, {"pfp_generations"});
        d = d.Define("n_pfp_gen_3", [](const ROOT::RVec<unsigned int>& generations) { return ROOT::VecOps::Sum(generations == 3); }, {"pfp_generations"});

        d = d.Define("quality_selector", "is_reco_fv && num_slices == 1 && selection_pass == 1 && optical_filter_pe_beam > 20.0");
        
        if (d.HasColumn("track_trunk_rr_dedx_u") && d.HasColumn("track_trunk_rr_dedx_v") && d.HasColumn("track_trunk_rr_dedx_y")) {
            auto avg_dedx_calculator = [](const ROOT::RVec<float>& u_dedx, const ROOT::RVec<float>& v_dedx, const ROOT::RVec<float>& y_dedx) {
                ROOT::RVec<float> avg_dedx(u_dedx.size());
                for (size_t i = 0; i < u_dedx.size(); ++i) {
                    float sum = 0.0; int count = 0;
                    if (u_dedx[i] > 0) { sum += u_dedx[i]; count++; }
                    if (v_dedx[i] > 0) { sum += v_dedx[i]; count++; }
                    if (y_dedx[i] > 0) { sum += y_dedx[i]; count++; }
                    avg_dedx[i] = (count > 0) ? sum / count : -1.0;
                }
                return avg_dedx;
            };
            d = d.Define("trk_trunk_rr_dedx_avg_v", avg_dedx_calculator, {"track_trunk_rr_dedx_u", "track_trunk_rr_dedx_v", "track_trunk_rr_dedx_y"});

            auto muon_candidate_mask = [this](const ROOT::RVec<float>& ts, const ROOT::RVec<float>& l, const ROOT::RVec<float>& dist, const ROOT::RVec<float>& trunk_rr_dedx_avg) {
                ROOT::RVec<bool> mask(ts.size());
                for (size_t i = 0; i < ts.size(); ++i) {
                    bool quality = (this->getElementFromVector(ts, i, 0.f) > 0.3f &&
                                    this->getElementFromVector(l, i, 0.f) > 5.0f && 
                                    this->getElementFromVector(trunk_rr_dedx_avg, i, 0.f) < 3.0f);
                    mask[i] = quality;
                }
                return mask;
            };
            d = d.Define("muon_candidate_mask", muon_candidate_mask, {"track_shower_scores", "track_length", "track_distance_to_vertex", "trk_trunk_rr_dedx_avg_v"});

            d = d.Define("n_muons", "ROOT::VecOps::Sum(muon_candidate_mask)");
            d = d.Define("muon_candidate_selector", "n_muons > 0");
        }

        return d;
    }
    
private:
    template<typename T_vec, typename T_val = typename T_vec::value_type>
    T_val getElementFromVector(const T_vec& vec, int index, T_val default_val = T_val{}) const {
        if (index >= 0 && static_cast<size_t>(index) < vec.size()) {
            return vec[index];
        }
        return default_val;
    }
};

} 
#endif