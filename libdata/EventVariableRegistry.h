#ifndef EVENT_VARIABLE_REGISTRY_H
#define EVENT_VARIABLE_REGISTRY_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SampleTypes.h"

namespace analysis {

class EventVariableRegistry {
public:
  using KnobVariations =
      std::unordered_map<std::string, std::pair<std::string, std::string>>;
  using MultiUniverseVars = std::unordered_map<std::string, unsigned>;

  static const KnobVariations &knobVariations() {
    static const KnobVariations m = {
        {"RPA", {"knobRPAup", "knobRPAdn"}},
        {"CCMEC", {"knobCCMECup", "knobCCMECdn"}},
        {"AxFFCCQE", {"knobAxFFCCQEup", "knobCCMECdn"}},
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
    static const MultiUniverseVars m = {{"weightsGenie", 500},
                                        {"weightsFlux", 500},
                                        {"weightsReint", 500},
                                        {"weightsPPFX", 500}};
    return m;
  }

  static const std::string &singleKnobVar() {
    static const std::string s = "RootinoFix";
    return s;
  }

  static std::vector<std::string> eventVariables(SampleOrigin type) {
    std::unordered_set<std::string> vars{baseVariables().begin(),
                                         baseVariables().end()};

    vars.insert(recoEventVariables().begin(), recoEventVariables().end());
    vars.insert(recoTrackVariables().begin(), recoTrackVariables().end());

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
                                               "backtracked_pdg_codes"};
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

  static const std::vector<std::string> &recoTrackVariables() {
    static const std::vector<std::string> v = {
        "track_length",        "track_distance_to_vertex",
        "track_start_x",       "track_start_y",
        "track_start_z",       "track_end_x",
        "track_end_y",         "track_end_z",
        "track_theta",         "track_phi",
        "track_calo_energy_u", "track_calo_energy_v",
        "track_calo_energy_y"};
    return v;
  }
};

}

#endif