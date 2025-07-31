#ifndef VARIABLEMANAGER_H
#define VARIABLEMANAGER_H

#include <string>
#include <vector>
#include <set>

#include "DataTypes.h"

namespace AnalysisFramework {

struct VariableOptions {
    bool load_blips = false;
};

class VariableManager {
private:
    std::vector<std::string> base_event_vars_ = {
        "run", "sub", "evt"
    };

    std::vector<std::string> truth_event_vars_ = {
        "neutrino_pdg", "interaction_ccnc", "interaction_mode", "interaction_type", "neutrino_energy", "lepton_energy",
        "count_mu_minus", "count_mu_plus", "count_e_minus", "count_e_plus", "count_pi_zero", "count_pi_plus", "count_pi_minus", "count_proton", "count_neutron",
        "count_kaon_plus", "count_kaon_minus", "count_kaon_zero", "count_lambda", "count_sigma_plus", "count_sigma_zero", "count_sigma_minus",
        "semantic_image_u", "semantic_image_v", "semantic_image_w",
        "neutrino_vertex_x", "neutrino_vertex_y", "neutrino_vertex_z",
        "neutrino_vertex_time", "neutrino_sce_vertex_x", "neutrino_sce_vertex_y", "neutrino_sce_vertex_z",
        "neutrino_completeness_from_pfp", "neutrino_purity_from_pfp",
        "target_nucleus_pdg", "hit_nucleon_pdg", "kinematic_W", "kinematic_X", "kinematic_Y", "kinematic_Q_squared"
    };

    std::vector<std::string> reco_event_vars_ = {
        "reco_neutrino_vertex_x", "reco_neutrino_vertex_y", "reco_neutrino_vertex_z",
        "reco_neutrino_vertex_sce_x", "reco_neutrino_vertex_sce_y", "reco_neutrino_vertex_sce_z",
        "num_slices", "slice_num_hits", "selection_pass", "slice_id", "slice_topological_scores",
        "optical_filter_pe_beam", "optical_filter_pe_veto",
        "num_pfps", "num_tracks", "num_showers", "pfp_num_hits",
        "detector_image_u", "detector_image_v", "detector_image_w",
        "event_total_hits", "slice_cluster_fraction",
        "total_hits_U", "total_hits_V", "total_hits_Y",
        "pfp_generations", "crt_veto", "crt_hit_pe", "software_trigger"
    };

    std::vector<std::string> reco_blip_vars_ = {
        "blip_ID",
        "blip_isValid",
        "blip_TPC",
        "blip_NPlanes",
        "blip_MaxWireSpan",
        "blip_Energy",
        "blip_EnergyESTAR",
        "blip_Time",
        "blip_ProxTrkDist",
        "blip_ProxTrkID",
        "blip_inCylinder",
        "blip_X",
        "blip_Y",
        "blip_Z",
        "blip_SigmaYZ",
        "blip_dX",
        "blip_dYZ",
        "blip_Charge",
        "blip_LeadG4ID",
        "blip_pdg",
        "blip_process",
        "blip_vx",
        "blip_vy",
        "blip_vz",
        "blip_E",
        "blip_mass",
        "blip_trkid"
    };

    std::vector<std::string> reco_track_vars_ = {
        "track_pfp_ids", "track_shower_scores", "track_length", "track_distance_to_vertex",
        "track_start_x", "track_start_y", "track_start_z",
        "track_end_x", "track_end_y", "track_end_z",
        "track_sce_start_x", "track_sce_start_y", "track_sce_start_z",
        "track_sce_end_x", "track_sce_end_y", "track_sce_end_z",
        "track_theta", "track_phi",
        "track_direction_x", "track_direction_y", "track_direction_z",
        "pfp_generations", "pfp_track_daughters", "pfp_shower_daughters",
        "track_nhits_u", "track_nhits_v", "track_nhits_y",
        "track_avg_deflection_stdev", "track_avg_deflection_mean", "track_avg_deflection_separation_mean",
        "track_end_spacepoints",
        "backtracked_pdg_codes", "backtracked_energies", "backtracked_purities", "backtracked_completenesses",
        "backtracked_overlay_purities", "backtracked_momentum_x", "backtracked_momentum_y", "backtracked_momentum_z",
        "backtracked_start_x", "backtracked_start_y", "backtracked_start_z", "backtracked_start_time",
        "backtracked_sce_start_x", "backtracked_sce_start_y", "backtracked_sce_start_z",
        "mc_particle_final_state",
        "track_calo_energy_u", "track_calo_energy_v", "track_calo_energy_y",
        "track_trunk_dedx_u", "track_trunk_dedx_v", "track_trunk_dedx_y",
        "track_trunk_rr_dedx_u", "track_trunk_rr_dedx_v", "track_trunk_rr_dedx_y"
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
        if (options.load_blips) {
            vars_set.insert(reco_blip_vars_.begin(), reco_blip_vars_.end());
        }

        return std::vector<std::string>(vars_set.begin(), vars_set.end());
    }
};

}

#endif