#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include <string>
#include <vector>
#include <set>

#include "DataTypes.h"

namespace AnalysisFramework {

struct VariableOptions {
    bool load_reco_event_info = false;
    bool load_reco_track_info = false;
    bool load_truth_event_info = true;
    bool load_weights_and_systematics = false;
    bool load_reco_shower_info = false;
    bool load_blip_info = false;
};

class VariableManager {
private:
    std::vector<std::string> base_event_vars_ = {"run", "sub", "evt"};

    std::vector<std::string> truth_event_vars_ = {
        "nu_pdg", "ccnc", "interaction", "nu_e", "lep_e",
        "mcf_nmm", "mcf_nmp", "mcf_nem", "mcf_nep", "mcf_np0", "mcf_npp", "mcf_npm", "mcf_npr", "mcf_nne",
        "mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
        //"true_image_u", "true_image_v", "true_image_w", 
        "true_nu_vtx_x", "true_nu_vtx_y", "true_nu_vtx_z"
    };

    std::vector<std::string> reco_event_vars_ = {
        "reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z",
        "nslice", "slnhits", "selected", "slice_id", "slice_topo_score_v",
        "_opfilter_pe_beam", "_opfilter_pe_veto",
        "n_pfps", "n_tracks", "n_showers",
        "detector_image_u", "detector_image_v", "detector_image_w",
        "evnhits", "slclustfrac"
    };

    std::vector<std::string> reco_track_vars_ = {
        "trk_pfp_id_v", "trk_score_v", "trk_len_v", "trk_distance_v", "trk_llr_pid_score_v",
        "trk_phi_v", "trk_theta_v", "trk_range_muon_mom_v", "trk_mcs_muon_mom_v",
        "trk_start_x_v", "trk_end_x_v", "trk_start_y_v", "trk_end_y_v", "trk_start_z_v", "trk_end_z_v"
    };

    std::vector<std::string> reco_shower_vars_ = {
        "shr_dedx_u_v", "shr_dedx_v_v", "shr_dedx_y_v",
        "shr_energy_u_v", "shr_energy_v_v", "shr_energy_y_v",
        "shr_pfp_id_v"
    };

    std::vector<std::string> blip_vars_ = {"blip_NPlanes"};

    std::vector<std::string> nominal_mc_weights_ = {
        "event_weight", "weights", "weightSpline", "weightTune", 
        "weightSplineTimesTune", "ppfx_cv", "nu_decay_mode"
    };

    std::vector<std::string> systematic_knob_weights_ = {
        "knobRPAup", "knobRPAdn", "knobCCMECup", "knobCCMECdn", "knobAxFFCCQEup", "knobAxFFCCQEdn",
        "knobVecFFCCQEup", "knobVecFFCCQEdn", "knobDecayAngMECup", "knobDecayAngMECdn",
        "knobThetaDelta2Npiup", "knobThetaDelta2Npidn", "knobThetaDelta2NRadup", "knobThetaDelta2NRaddn",
        "knobNormCCCOHup", "knobNormCCCOHdn", "knobNormNCCOHup", "knobNormNCCOHdn",
        "knobxsr_scc_Fv3up", "knobxsr_scc_Fv3dn", "knobxsr_scc_Fa3up", "knobxsr_scc_Fa3dn", "RootinoFix"
    };

    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> knob_variations_ = {
        {"RPA", {"knobRPAup", "knobRPAdn"}},
        {"CCMEC", {"knobCCMECup", "knobCCMECdn"}},
        {"AxFFCCQE", {"knobAxFFCCQEup", "knobAxFFCCQEdn"}},
        {"VecFFCCQE", {"knobVecFFCCQEup", "knobVecFFCCQEdn"}},
        {"DecayAngMEC", {"knobDecayAngMECup", "knobDecayAngMECdn"}},
        {"ThetaDelta2Npi", {"knobThetaDelta2Npiup", "knobThetaDelta2Npidn"}},
        {"ThetaDelta2NRad", {"knobThetaDelta2NRadup", "knobThetaDelta2NRaddn"}},
        {"NormCCCOH", {"knobNormCCCOHup", "knobNormCCCOHdn"}},
        {"NormNCCOH", {"knobNormNCCOHup", "knobNormNCCOHdn"}},
        {"xsr_scc_Fv3", {"knobxsr_scc_Fv3up", "knobxsr_scc_Fv3dn"}},
        {"xsr_scc_Fa3", {"knobxsr_scc_Fa3up", "knobxsr_scc_Fa3dn"}}
    };

    std::string single_variation_variable_ = "RootinoFix";
    
    std::vector<std::string> multi_universe_weights_ = {"weightsGenie", "weightsFlux", "weightsReint", "weightsPPFX"};

public:
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> GetKnobVariations() const {
        return knob_variations_;
    }

    std::string GetSingleKnobVariation() const {
        return single_variation_variable_;
    }

    std::vector<std::string> GetVariables(const VariableOptions& options, AnalysisFramework::SampleType sample_type) const {
        std::set<std::string> vars_set;
        vars_set.insert(base_event_vars_.begin(), base_event_vars_.end());
        if (options.load_truth_event_info && sample_type == SampleType::kMonteCarlo) {
            vars_set.insert(truth_event_vars_.begin(), truth_event_vars_.end());
        }
        if (options.load_reco_event_info) {
            vars_set.insert(reco_event_vars_.begin(), reco_event_vars_.end());
        }
        if (options.load_reco_track_info) {
            vars_set.insert(reco_track_vars_.begin(), reco_track_vars_.end());
        }
        if (options.load_weights_and_systematics && sample_type == SampleType::kMonteCarlo) {
            vars_set.insert(nominal_mc_weights_.begin(), nominal_mc_weights_.end());
            vars_set.insert(systematic_knob_weights_.begin(), systematic_knob_weights_.end());
            vars_set.insert(multi_universe_weights_.begin(), multi_universe_weights_.end());
        }
        if (options.load_reco_shower_info) {
            vars_set.insert(reco_shower_vars_.begin(), reco_shower_vars_.end());
        }
        if (options.load_blip_info) {
            vars_set.insert(blip_vars_.begin(), blip_vars_.end());
        }
        return std::vector<std::string>(vars_set.begin(), vars_set.end());
    }

    std::map<std::string, unsigned int> GetMultiUniverseDefinitions() const {
        return {
            {"weightsGenie", 500},
            {"weightsFlux", 500},
            {"weightsReint", 500},
            {"weightsPPFX", 500}
        };
    }
};

}

#endif