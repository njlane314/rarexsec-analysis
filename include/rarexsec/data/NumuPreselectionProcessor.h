#ifndef NUMU_PRESELECTION_PROCESSOR_H
#define NUMU_PRESELECTION_PROCESSOR_H

#include <rarexsec/data/IEventProcessor.h>
#include <rarexsec/data/SampleTypes.h>
#include "ROOT/RVec.hxx"

namespace analysis {

class NumuPreselectionProcessor : public IEventProcessor {
public:
  ROOT::RDF::RNode process(ROOT::RDF::RNode df,
                           SampleOrigin st) const override {
    auto proc_df =
        df.Define("nslice", "num_slices")
            .Define("_opfilter_pe_beam", "optical_filter_pe_beam")
            .Define("_opfilter_pe_veto", "optical_filter_pe_veto")
            .Define("reco_nu_vtx_sce_x", "reco_neutrino_vertex_sce_x")
            .Define("reco_nu_vtx_sce_y", "reco_neutrino_vertex_sce_y")
            .Define("reco_nu_vtx_sce_z", "reco_neutrino_vertex_sce_z")
            .Define("n_pfps_gen2",
                    [](const ROOT::RVec<unsigned> &gens) {
                        return ROOT::VecOps::Sum(gens == 2u);
                    },
                    {"pfp_generations"});

    if (st == SampleOrigin::kMonteCarlo) {
        if (proc_df.HasColumn("software_trigger_pre_ext")) {
            proc_df = proc_df.Define(
                "software_trigger",
                [](unsigned run, int pre, int post) {
                    return run < 16880 ? pre > 0 : post > 0;
                },
                {"run", "software_trigger_pre_ext", "software_trigger_post_ext"});
        } else if (proc_df.HasColumn("software_trigger_pre")) {
            proc_df = proc_df.Define(
                "software_trigger",
                [](unsigned run, int pre, int post) {
                    return run < 16880 ? pre > 0 : post > 0;
                },
                {"run", "software_trigger_pre", "software_trigger_post"});
        } else if (!proc_df.HasColumn("software_trigger")) {
            proc_df = proc_df.Define("software_trigger", []() { return true; });
        }
    } else {
        proc_df = proc_df.Define("software_trigger", []() { return true; });
    }

    proc_df = proc_df
                 .Define("bnbdata", [st]() { return st == SampleOrigin::kData ? 1 : 0; })
                 .Define("extdata", [st]() { return st == SampleOrigin::kExternal ? 1 : 0; });

    auto presel_df = proc_df.Define(
        "numu_presel",
        [st](int bnb, int ext, float pe_beam, float pe_veto, bool swtrig,
             int nslice, float topo, int n_gen2, float x, float y, float z) {
            bool dataset_gate = (bnb == 0 && ext == 0) ?
                                    (pe_beam > 0.f && pe_veto < 20.f) :
                                    true;
            bool basic_reco = nslice == 1 && topo > 0.06f && n_gen2 > 1;
            bool fv = x > 5.f && x < 251.f && y > -110.f && y < 110.f &&
                      z > 20.f && z < 986.f && (z < 675.f || z > 775.f);
            return dataset_gate &&
                   (st == SampleOrigin::kMonteCarlo ? swtrig : true) &&
                   basic_reco && fv;
        },
        {"bnbdata", "extdata", "_opfilter_pe_beam", "_opfilter_pe_veto",
         "software_trigger", "nslice", "topological_score", "n_pfps_gen2",
         "reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y",
         "reco_nu_vtx_sce_z"});

    return next_ ? next_->process(presel_df, st) : presel_df;
  }
};

} // namespace analysis

#endif
