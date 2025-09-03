#ifndef EVENT_VARIABLE_REGISTRY_H
#define EVENT_VARIABLE_REGISTRY_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SampleTypes.h"

namespace analysis {

class VariableRegistry {
  public:
    using KnobVariations = std::unordered_map<std::string, std::pair<std::string, std::string>>;
    using MultiUniverseVars = std::unordered_map<std::string, unsigned>;

    static const KnobVariations &knobVariations() {
        static const KnobVariations m = {{"RPA", {"knobRPAup", "knobRPAdn"}},
                                         {"CCMEC", {"knobCCMECup", "knobCCMECdn"}},
                                         {"AxFFCCQE", {"knobAxFFCCQEup", "knobAxFFCCQEdn"}},
                                         {"VecFFCCQE", {"knobVecFFCCQEup", "knobVecFFCCQEdn"}},
                                         {"DecayAngMEC", {"knobDecayAngMECup", "knobDecayAngMECdn"}},
                                         {"ThetaDelta2Npi", {"knobThetaDelta2Npiup", "knobThetaDelta2Npidn"}},
                                         {"ThetaDelta2NRad", {"knobThetaDelta2NRadup", "knobThetaDelta2NRaddn"}},
                                         {"NormCCCOH", {"knobNormCCCOHup", "knobNormCCCOHdn"}},
                                         {"NormNCCOH", {"knobNormNCCOHup", "knobNormNCCOHdn"}},
                                         {"xsr_scc_Fv3", {"knobxsr_scc_Fv3up", "knobxsr_scc_Fv3dn"}},
                                         {"xsr_scc_Fa3", {"knobxsr_scc_Fa3up", "knobxsr_scc_Fa3dn"}}};
        return m;
    }

    static const MultiUniverseVars &multiUniverseVariations() {
        static const MultiUniverseVars m = {
            {"weightsGenie", 500}, {"weightsFlux", 500}, {"weightsReint", 500}, {"weightsPPFX", 500}};
        return m;
    }

    static const std::string &singleKnobVar() {
        static const std::string s = "RootinoFix";
        return s;
    }

    static std::vector<std::string> eventVariables(SampleOrigin type) {
        std::unordered_set<std::string> vars{baseVariables().begin(), baseVariables().end()};

        vars.insert(recoEventVariables().begin(), recoEventVariables().end());
        vars.insert(recoTrackVariables().begin(), recoTrackVariables().end());
        vars.insert(processedEventVariables().begin(), processedEventVariables().end());
        vars.insert(blipVariables().begin(), blipVariables().end());
        vars.insert(imageVariables().begin(), imageVariables().end());
        vars.insert(flashVariables().begin(), flashVariables().end());
        vars.insert(energyVariables().begin(), energyVariables().end());
        vars.insert(sliceVariables().begin(), sliceVariables().end());

        if (type == SampleOrigin::kMonteCarlo) {
            vars.insert(truthVariables().begin(), truthVariables().end());

            for (auto &kv : knobVariations()) {
                vars.insert(kv.second.first);
                vars.insert(kv.second.second);
            }

            for (auto &kv : multiUniverseVariations()) {
                vars.insert(kv.first);
            }

            vars.insert(singleKnobVar());
        }

        return {vars.begin(), vars.end()};
    }

  private:
    static const std::vector<std::string> &baseVariables() {
        static const std::vector<std::string> v = {"run", "sub", "evt"};
        return v;
    }

    static const std::vector<std::string> &truthVariables() {
        static const std::vector<std::string> v = {"neutrino_pdg",
                                                   "interaction_ccnc",
                                                   "interaction_mode",
                                                   "interaction_type",
                                                   "neutrino_energy",
                                                   "lepton_energy",
                                                   "count_mu_minus",
                                                   "count_mu_plus",
                                                   "count_e_minus",
                                                   "count_e_plus",
                                                   "count_pi_zero",
                                                   "count_pi_plus",
                                                   "count_pi_minus",
                                                   "count_proton",
                                                   "count_neutron",
                                                   "count_kaon_zero",
                                                   "count_kaon_plus",
                                                   "count_kaon_minus",
                                                   "count_lambda",
                                                   "count_sigma_zero",
                                                   "count_sigma_plus",
                                                   "count_sigma_minus",
                                                   "neutrino_vertex_x",
                                                   "neutrino_vertex_y",
                                                   "neutrino_vertex_z",
                                                   "neutrino_vertex_time",
                                                   "neutrino_completeness_from_pfp",
                                                   "neutrino_purity_from_pfp",
                                                   "target_nucleus_pdg",
                                                   "hit_nucleon_pdg",
                                                   "kinematic_W",
                                                   "kinematic_X",
                                                   "kinematic_Y",
                                                   "kinematic_Q_squared",
                                                   "backtracked_pdg_codes",
                                                   "blip_pdg"};
        return v;
    }

    static const std::vector<std::string> &recoEventVariables() {
        static const std::vector<std::string> v = {"reco_neutrino_vertex_sce_x",
                                                   "reco_neutrino_vertex_sce_y",
                                                   "reco_neutrino_vertex_sce_z",
                                                   "num_slices",
                                                   "slice_num_hits",
                                                   "selection_pass",
                                                   "slice_id",
                                                   "optical_filter_pe_beam",
                                                   "optical_filter_pe_veto",
                                                   "num_pfps",
                                                   "num_tracks",
                                                   "num_showers",
                                                   "event_total_hits"};
        return v;
    }

    static const std::vector<std::string> &blipVariables() {
        static const std::vector<std::string> v = {"blip_id",
                                                   "blip_is_valid",
                                                   "blip_tpc",
                                                   "blip_n_planes",
                                                   "blip_max_wire_span",
                                                   "blip_energy",
                                                   "blip_energy_estar",
                                                   "blip_time",
                                                   "blip_prox_trk_dist",
                                                   "blip_prox_trk_id",
                                                   "blip_in_cylinder",
                                                   "blip_x",
                                                   "blip_y",
                                                   "blip_z",
                                                   "blip_sigma_yz",
                                                   "blip_dx",
                                                   "blip_dyz",
                                                   "blip_charge",
                                                   "blip_lead_g4_id",
                                                   "blip_pdg",
                                                   "blip_process",
                                                   "blip_process_code",
                                                   "blip_vx",
                                                   "blip_vy",
                                                   "blip_vz",
                                                   "blip_e",
                                                   "blip_mass",
                                                   "blip_trk_id",
                                                   "blip_distance_to_vertex"};
        return v;
    }

    static const std::vector<std::string> &imageVariables() {
        static const std::vector<std::string> v = {"reco_neutrino_vertex_x",
                                                   "reco_neutrino_vertex_y",
                                                   "reco_neutrino_vertex_z",
                                                   "detector_image_u",
                                                   "detector_image_v",
                                                   "detector_image_w",
                                                   "semantic_image_u",
                                                   "semantic_image_v",
                                                   "semantic_image_w",
                                                   "event_detector_image_u",
                                                   "event_detector_image_v",
                                                   "event_detector_image_w",
                                                   "event_semantic_image_u",
                                                   "event_semantic_image_v",
                                                   "event_semantic_image_w",
                                                   "event_adc_u",
                                                   "event_adc_v",
                                                   "event_adc_w",
                                                   "slice_semantic_counts_u",
                                                   "slice_semantic_counts_v",
                                                   "slice_semantic_counts_w",
                                                   "event_semantic_counts_u",
                                                   "event_semantic_counts_v",
                                                   "event_semantic_counts_w",
                                                   "is_vtx_in_image_u",
                                                   "is_vtx_in_image_v",
                                                   "is_vtx_in_image_w",
                                                   "inference_score"};
        return v;
    }

    static const std::vector<std::string> &flashVariables() {
        static const std::vector<std::string> v = {"t0",
                                                   "flash_match_score",
                                                   "flash_total_pe",
                                                   "flash_time",
                                                   "flash_z_center",
                                                   "flash_z_width",
                                                   "slice_charge",
                                                   "slice_z_center",
                                                   "charge_light_ratio",
                                                   "flash_slice_z_dist",
                                                   "flash_pe_per_charge"};
        return v;
    }

    static const std::vector<std::string> &energyVariables() {
        static const std::vector<std::string> v = {"neutrino_energy_0",
                                                   "neutrino_energy_1",
                                                   "neutrino_energy_2",
                                                   "slice_calo_energy_0",
                                                   "slice_calo_energy_1",
                                                   "slice_calo_energy_2"};
        return v;
    }

    static const std::vector<std::string> &sliceVariables() {
        static const std::vector<std::string> v = {"original_event_neutrino_hits",
                                                   "event_neutrino_hits",
                                                   "event_muon_hits",
                                                   "event_electron_hits",
                                                   "event_proton_hits",
                                                   "event_charged_pion_hits",
                                                   "event_neutral_pion_hits",
                                                   "event_neutron_hits",
                                                   "event_gamma_hits",
                                                   "event_other_hits",
                                                   "event_charged_kaon_hits",
                                                   "event_neutral_kaon_hits",
                                                   "event_lambda_hits",
                                                   "event_charged_sigma_hits",
                                                   "event_sigma_zero_hits",
                                                   "event_cosmic_hits",
                                                   "slice_neutrino_hits",
                                                   "slice_muon_hits",
                                                   "slice_electron_hits",
                                                   "slice_proton_hits",
                                                   "slice_charged_pion_hits",
                                                   "slice_neutral_pion_hits",
                                                   "slice_neutron_hits",
                                                   "slice_gamma_hits",
                                                   "slice_other_hits",
                                                   "slice_charged_kaon_hits",
                                                   "slice_neutral_kaon_hits",
                                                   "slice_lambda_hits",
                                                   "slice_charged_sigma_hits",
                                                   "slice_sigma_zero_hits",
                                                   "slice_cosmic_hits",
                                                   "pfp_neutrino_hits",
                                                   "pfp_muon_hits",
                                                   "pfp_electron_hits",
                                                   "pfp_proton_hits",
                                                   "pfp_charged_pion_hits",
                                                   "pfp_neutral_pion_hits",
                                                   "pfp_neutron_hits",
                                                   "pfp_gamma_hits",
                                                   "pfp_other_hits",
                                                   "pfp_charged_kaon_hits",
                                                   "pfp_neutral_kaon_hits",
                                                   "pfp_lambda_hits",
                                                   "pfp_charged_sigma_hits",
                                                   "pfp_sigma_zero_hits",
                                                   "pfp_cosmic_hits",
                                                   "neutrino_completeness_from_pfp",
                                                   "neutrino_purity_from_pfp"};
        return v;
    }

    static const std::vector<std::string> &recoTrackVariables() {
        static const std::vector<std::string> v = {"track_length",        "track_distance_to_vertex",
                                                   "track_start_x",       "track_start_y",
                                                   "track_start_z",       "track_end_x",
                                                   "track_end_y",         "track_end_z",
                                                   "track_theta",         "track_phi",
                                                   "track_calo_energy_u", "track_calo_energy_v",
                                                   "track_calo_energy_y"};
        return v;
    }

    static const std::vector<std::string> &processedEventVariables() {
        static const std::vector<std::string> v = {
            "in_reco_fiducial", "n_pfps_gen2",       "n_pfps_gen3",         "quality_event",     "n_muons",
            "has_muon",         "muon_track_length", "muon_track_costheta", "base_event_weight", "nominal_event_weight",
            "in_fiducial",      "mc_n_strange",      "mc_n_pion",           "mc_n_proton",       "genie_int_mode",
            "incl_channel",     "excl_channel"};
        return v;
    }
};

}

#endif
