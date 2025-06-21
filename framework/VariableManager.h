#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include <string>
#include <vector>
#include <set>

#include "DataTypes.h"

namespace AnalysisFramework {

struct VariableOptions {
    bool load_showers = false;
};

class VariableManager {
private:
    std::vector<std::string> base_event_vars_ = {
        "run", "sub", "evt"
    };

    std::vector<std::string> truth_event_vars_ = {
        "nu_pdg", "ccnc", "interaction", "nu_e", "lep_e",
        "mcf_nmm", "mcf_nmp", "mcf_nem", "mcf_nep", "mcf_np0", "mcf_npp", "mcf_npm", "mcf_npr", "mcf_nne",
        "mcf_nkp", "mcf_nkm", "mcf_nk0", "mcf_nlambda", "mcf_nsigma_p", "mcf_nsigma_0", "mcf_nsigma_m",
        "semantic_image_u", "semantic_image_v", "semantic_image_w", 
        "true_nu_vtx_x", "true_nu_vtx_y", "true_nu_vtx_z",
        "true_nu_vtx_t", "true_nu_vtx_sce_x", "true_nu_vtx_sce_y", "true_nu_vtx_sce_z"
    };

    std::vector<std::string> reco_event_vars_ = {
        "reco_nu_vtx_x", "reco_nu_vtx_y", "reco_nu_vtx_z",
        "reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z",
        "nslice", "slnhits", "selected", "slice_id", "slice_topo_score_v",
        "_opfilter_pe_beam", "_opfilter_pe_veto",
        "n_pfps", "n_tracks", "n_showers", "pfnhits",
        "detector_image_u", "detector_image_v", "detector_image_w",
        "evnhits", "slclustfrac",
        "nhits_u", "charge_u", "wirerange_u", "timerange_u",
        "nhits_v", "charge_v", "wirerange_v", "timerange_v",
        "nhits_w", "charge_w", "wirerange_w", "timerange_w",
        "semantic_salience",
        "nclusters_u", "nclusters_v", "nclusters_w",
        "pfp_generation_v"
    };

    std::vector<std::string> reco_track_vars_ = {
        "trk_pfp_id_v", "trk_score_v", "trk_len_v", "trk_distance_v",
        "trk_start_x_v", "trk_start_y_v", "trk_start_z_v",
        "trk_end_x_v", "trk_end_y_v", "trk_end_z_v",
        "trk_sce_start_x_v", "trk_sce_start_y_v", "trk_sce_start_z_v",
        "trk_sce_end_x_v", "trk_sce_end_y_v", "trk_sce_end_z_v",
        "trk_theta_v", "trk_phi_v",
        "trk_dir_x_v", "trk_dir_y_v", "trk_dir_z_v",
        "generation_v", "trk_daughters_v", "shr_daughters_v", "longest_v",
        "trk_nhits_u_v", "trk_nhits_v_v", "trk_nhits_y_v",
        "trk_llr_pid_score_v",
        "trk_pida_v",
        "trk_pid_chimu_v", "trk_pid_chipr_v", "trk_pid_chika_v", "trk_pid_chipi_v",
        "trk_trunk_dEdx_y_v", "trk_trunk_rr_dEdx_y_v",
        "trk_range_muon_mom_v", "trk_mcs_muon_mom_v",
        "trk_energy_muon_v", "trk_energy_proton_v",
        "trk_avg_deflection_stdev_v", "trk_avg_deflection_mean_v", "trk_avg_deflection_separation_mean_v",
        "trk_end_spacepoints_v",
        "backtracked_pdg", "backtracked_e", "backtracked_purity", "backtracked_completeness",
        "backtracked_overlay_purity", "backtracked_px", "backtracked_py", "backtracked_pz",
        "backtracked_start_x", "backtracked_start_y", "backtracked_start_z", "backtracked_start_t",
        "backtracked_sce_start_x", "backtracked_sce_start_y", "backtracked_sce_start_z",
        "backtracked_end_process", "backtracked_end_in_tpc"
    };

    std::vector<std::string> reco_shower_vars_ = {
        "shr_dedx_u_v", "shr_dedx_v_v", "shr_dedx_y_v",
        "shr_energy_u_v", "shr_energy_v_v", "shr_energy_y_v",
        "shr_pfp_id_v",
        "shr_tkfit_dedx_y_v", "shr_moliere_avg_v", "shr_openangle_v"
    };

    std::vector<std::string> nominal_mc_weights_ = {
        "weightSpline", "weightTune", "ppfx_cv"
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
    std::vector<std::pair<std::string, std::pair<std::string, std::string>>> getKnobVariations() const {
        return knob_variations_;
    }

    std::string getSingleKnobVariation() const {
        return single_variation_variable_;
    }

    std::map<std::string, unsigned int> getMultiUniverseDefinitions() const {
        return {
            {"weightsGenie", 500},
            {"weightsFlux", 500},
            {"weightsReint", 500},
            {"weightsPPFX", 500}
        };
    }

    std::vector<std::string> getVariables(const VariableOptions& options, AnalysisFramework::SampleType sample_type) const {
        std::set<std::string> vars_set;

        vars_set.insert(base_event_vars_.begin(), base_event_vars_.end());
        if (sample_type == SampleType::kMonteCarlo) {
            vars_set.insert(truth_event_vars_.begin(), truth_event_vars_.end());
            vars_set.insert(nominal_mc_weights_.begin(), nominal_mc_weights_.end());
            vars_set.insert(systematic_knob_weights_.begin(), systematic_knob_weights_.end());
            vars_set.insert(multi_universe_weights_.begin(), multi_universe_weights_.end());
        }
     
        vars_set.insert(reco_event_vars_.begin(), reco_event_vars_.end());
        vars_set.insert(reco_track_vars_.begin(), reco_track_vars_.end());
        if (options.load_showers) {
            vars_set.insert(reco_shower_vars_.begin(), reco_shower_vars_.end());
        }
    
        return std::vector<std::string>(vars_set.begin(), vars_set.end());
    }
};

}

#endif