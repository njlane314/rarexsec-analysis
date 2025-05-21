#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include <string>
#include <vector>
#include <set>

#include "SampleTypes.h"

namespace AnalysisFramework {

struct VariableOptions {
    bool load_reco_event_info = false;
    bool load_reco_track_info = false;
    bool load_truth_event_info = false;
    bool load_weights_and_systematics = false;
    bool load_signal_weights = false;
};

class VariableManager {
public:
    VariableManager() {
        base_event_vars_ = {"run", "subrun", "event"};

        truth_event_vars_ = {
            "nu_pdg", "nu_ccnc", "nu_mode", "nu_interaction", "nu_e", "nu_theta", "nu_pt", "nu_target_nucleus", "nu_hit_nucleon",
            "nu_W", "nu_X", "nu_Y", "nu_QSqr", "nu_px", "nu_py", "nu_pz", "nu_vtx_x_true", "nu_vtx_y_true", "nu_vtx_z_true",
            "mcf_nmm", "mcf_nmp", "mcf_nem", "mcf_nep", "mcf_np0", "mcf_npp", "mcf_npm", "mcf_npr", "mcf_nne",
            "mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
            "mcf_nxi_0", "mcf_nxi_m", "mcf_nomega",
            "true_image_u", "true_image_v", "true_image_v"  
        };

        reco_event_vars_ = {
            "reco_nu_vtx_x", "reco_nu_vtx_y", "reco_nu_vtx_z", "topological_score",
            "raw_image_u", "raw_image_v", "raw_image_w", 
            "reco_image_u", "reco_image_v", "reco_image_w"  
        };

        reco_track_vars_ = {
            "trk_pfp_id_v", "trk_score_v", "trk_len_v", "trk_distance_v", "trk_theta_v", "trk_phi_v",
            "trk_llr_pid_sum_v", "trk_llr_pid_score_v", "trk_mcs_muon_mom_v", "trk_range_muon_mom_v", "trk_generation_v",
            "trkpid_v"
        };

        nominal_mc_weights_ = {"weights", "weightSpline", "weightTune", "weightSplineTimesTune", "ppfx_cv", "nu_decay_mode"};

        systematic_knob_weights_ = {
            "knobRPAup", "knobRPAdn", "knobCCMECup", "knobCCMECdn", "knobAxFFCCQEup", "knobAxFFCCQEdn",
            "knobVecFFCCQEup", "knobVecFFCCQEdn", "knobDecayAngMECup", "knobDecayAngMECdn",
            "knobThetaDelta2Npiup", "knobThetaDelta2Npidn", "knobThetaDelta2NRadup", "knobThetaDelta2NRaddn",
            "knobNormCCCOHup", "knobNormCCCOHdn", "knobNormNCCOHup", "knobNormNCCOHdn",
            "knobxsr_scc_Fv3up", "knobxsr_scc_Fv3dn", "knobxsr_scc_Fa3up", "knobxsr_scc_Fa3dn", "RootinoFix"
        };

        multi_universe_weights_ = {"weightsGenie", "weightsFlux", "weightsReint", "weightsPPFX"};

        signal_weights = {};
    }

    std::vector<std::string> GetVariables(const VariableOptions& options, AnalysisFramework::SampleType sample_type) const {
        std::set<std::string> vars_set;
        vars_set.insert(base_event_vars_.begin(), base_event_vars_.end());

        if (options.load_truth_event_info && AnalysisFramework::is_sample_mc(sample_type)) {
            vars_set.insert(truth_event_vars_.begin(), truth_event_vars_.end());
        }
        if (options.load_reco_event_info) {
            vars_set.insert(reco_event_vars_.begin(), reco_event_vars_.end());
        }
        if (options.load_reco_track_info) {
            vars_set.insert(reco_track_vars_.begin(), reco_track_vars_.end());
        }

        if (options.load_weights_and_systematics && AnalysisFramework::is_sample_mc(sample_type) && !AnalysisFramework::is_sample_detvar(sample_type)) {
            vars_set.insert(nominal_mc_weights_.begin(), nominal_mc_weights_.end());
            vars_set.insert(systematic_knob_weights_.begin(), systematic_knob_weights_.end());
            vars_set.insert(multi_universe_weights_.begin(), multi_universe_weights_.end());
        }
        if (options.load_signal_weights && AnalysisFramework::is_sample_strange(sample_type)) {
            vars_set.insert(signal_weights.begin(), signal_weights.end());
        }

        return std::vector<std::string>(vars_set.begin(), vars_set.end());
    }

private:
    std::vector<std::string> base_event_vars_;
    std::vector<std::string> truth_event_vars_;
    std::vector<std::string> reco_event_vars_;
    std::vector<std::string> reco_track_vars_;

    std::vector<std::string> nominal_mc_weights_;
    std::vector<std::string> systematic_knob_weights_;
    std::vector<std::string> multi_universe_weights_;
    std::vector<std::string> signal_weights;
};

}

#endif