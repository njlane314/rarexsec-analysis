#ifndef NUMU_PRESELECTION_PROCESSOR_H
#define NUMU_PRESELECTION_PROCESSOR_H

#include <rarexsec/data/IEventProcessor.h>
#include <rarexsec/data/SampleTypes.h>

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
            .Define("reco_nu_vtx_sce_z", "reco_neutrino_vertex_sce_z");

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
        [st](int nslice, float pe_beam, float pe_veto, float x, float y, float z,
             float topo, int bnb, int ext, bool swtrig) {
            return nslice == 1 && ((pe_beam > 0 && pe_veto < 20) || bnb == 1 ||
                                   ext == 1) &&
                   x > 5 && x < 251 && y > -110 && y < 110 &&
                   (z < 675 || z > 775) && topo > 0.06 &&
                   (st == SampleOrigin::kMonteCarlo ? swtrig : true);
        },
        {"nslice", "_opfilter_pe_beam", "_opfilter_pe_veto",
         "reco_nu_vtx_sce_x", "reco_nu_vtx_sce_y", "reco_nu_vtx_sce_z",
         "topological_score", "bnbdata", "extdata", "software_trigger"});

    return next_ ? next_->process(presel_df, st) : presel_df;
  }
};

} // namespace analysis

#endif
