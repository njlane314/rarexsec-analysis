#ifndef NUMU_CC_SELECTION_PROCESSOR_H
#define NUMU_CC_SELECTION_PROCESSOR_H

#include <string>

#include <rarexsec/data/IEventProcessor.h>

namespace analysis {

/// Define boolean columns indicating whether an event passes each stage of the
/// muon-neutrino charged-current selection. Each stage also produces a string
/// column describing the reason for failure (empty string when the stage is
/// passed).
class NuMuCCSelectionProcessor : public IEventProcessor {
public:
  ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleOrigin st) const override {
    // Dataset and trigger gates
    auto pre_df = df.Define(
        "pass_pre",
        [st](int bnb, int ext, float pe_beam, float pe_veto, bool swtrig) {
          bool dataset_gate = (bnb == 0 && ext == 0) ?
                                 (pe_beam > 0.f && pe_veto < 20.f) : true;
          return dataset_gate && swtrig;
        },
        {"bnbdata", "extdata", "_opfilter_pe_beam", "_opfilter_pe_veto",
         "software_trigger"});

    // Basic reconstruction checks
    auto flash_df = pre_df
        .Define(
            "pass_flash",
            [](int nslice, float topo, int n_gen2) {
              return nslice == 1 && topo > 0.06f && n_gen2 > 1;
            },
            {"nslice", "topological_score", "n_pfps_gen2"})
        .Define(
            "reason_flash",
            [](int nslice, float topo, int n_gen2) {
              if (nslice != 1)
                return std::string{"nslice"};
              if (topo <= 0.06f)
                return std::string{"topological_score"};
              if (n_gen2 <= 1)
                return std::string{"n_pfps_gen2"};
              return std::string{};
            },
            {"nslice", "topological_score", "n_pfps_gen2"});

    // Neutrino-vertex fiducial volume
    auto fv_df = flash_df
        .Define(
            "pass_fv",
            [](float x, float y, float z) {
              return x > 5.f && x < 251.f && y > -110.f && y < 110.f &&
                     z > 20.f && z < 986.f && (z < 675.f || z > 775.f);
            },
            {"reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z"})
        .Define(
            "reason_fv",
            [](float x, float y, float z) {
              if (x <= 5.f || x >= 251.f)
                return std::string{"x"};
              if (y <= -110.f || y >= 110.f)
                return std::string{"y"};
              if (z <= 20.f || z >= 986.f || (z > 675.f && z < 775.f))
                return std::string{"z"};
              return std::string{};
            },
            {"reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z"});

    // Muon candidate requirements
    auto mu_df = fv_df
        .Define("pass_mu", "n_muons_tot > 0")
        .Define("reason_mu",
                [](int nmu) {
                  return nmu > 0 ? std::string{} : std::string{"no_muon"};
                },
                {"n_muons_tot"});

    // Slice-level quality requirements
    auto topo_df = mu_df
        .Define(
            "pass_topo",
            [](float contained, float cluster) {
              return contained >= 0.7f && cluster >= 0.5f;
            },
            {"contained_fraction", "slice_cluster_fraction"})
        .Define(
            "reason_topo",
            [](float contained, float cluster) {
              if (contained < 0.7f)
                return std::string{"contained_fraction"};
              if (cluster < 0.5f)
                return std::string{"slice_cluster_fraction"};
              return std::string{};
            },
            {"contained_fraction", "slice_cluster_fraction"});

    // Final event pass (all previous stages)
    auto final_df = topo_df
        .Define(
            "pass_final",
            [](bool pre, bool flash, bool fv, bool mu, bool topo) {
              return pre && flash && fv && mu && topo;
            },
            {"pass_pre", "pass_flash", "pass_fv", "pass_mu", "pass_topo"})
        .Define(
            "reason_final",
            [](bool pre, bool flash, bool fv, bool mu, bool topo) {
              if (!(pre && flash && fv && mu && topo))
                return std::string{"precondition"};
              return std::string{};
            },
            {"pass_pre", "pass_flash", "pass_fv", "pass_mu", "pass_topo"});

    return next_ ? next_->process(final_df, st) : final_df;
  }
};

} // namespace analysis

#endif

