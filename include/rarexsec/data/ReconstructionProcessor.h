#ifndef RECONSTRUCTION_PROCESSOR_H
#define RECONSTRUCTION_PROCESSOR_H

#include "ROOT/RVec.hxx"

#include <rarexsec/data/IEventProcessor.h>

namespace analysis {

class ReconstructionProcessor : public IEventProcessor {
  public:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleOrigin st) const override {
        auto fid_df = df.Define("in_reco_fiducial", "reco_neutrino_vertex_sce_x > 5 && "
                                                    "reco_neutrino_vertex_sce_x < 251 && "
                                                    "reco_neutrino_vertex_sce_y > -110 && "
                                                    "reco_neutrino_vertex_sce_y < 110 && "
                                                    "reco_neutrino_vertex_sce_z > 20 && "
                                                    "reco_neutrino_vertex_sce_z < 986");

        auto gen2_df =
            fid_df.Define("n_pfps_gen2", [](const ROOT::RVec<unsigned> &gens) { return ROOT::VecOps::Sum(gens == 2u); },
                          {"pfp_generations"});

        auto gen3_df = gen2_df.Define("n_pfps_gen3",
                                      [](const ROOT::RVec<unsigned> &gens) { return ROOT::VecOps::Sum(gens == 3u); },
                                      {"pfp_generations"});

        ROOT::RDF::RNode swtrig_df(gen3_df);
        if (st == SampleOrigin::kMonteCarlo) {
            if (gen3_df.HasColumn("software_trigger_pre_ext")) {
                swtrig_df = gen3_df.Define(
                    "software_trigger",
                    [](unsigned run, int pre, int post) {
                        return run < 16880 ? pre > 0 : post > 0;
                    },
                    {"run", "software_trigger_pre_ext", "software_trigger_post_ext"});
            } else if (gen3_df.HasColumn("software_trigger_pre")) {
                swtrig_df = gen3_df.Define(
                    "software_trigger",
                    [](unsigned run, int pre, int post) {
                        return run < 16880 ? pre > 0 : post > 0;
                    },
                    {"run", "software_trigger_pre", "software_trigger_post"});
            } else if (!gen3_df.HasColumn("software_trigger")) {
                swtrig_df = gen3_df.Define("software_trigger", []() { return true; });
            }
        } else {
            swtrig_df = gen3_df.Define("software_trigger", []() { return true; });
        }

        auto quality_df = swtrig_df.Define(
            "quality_event",
            [st](bool in_fid, int nslices, bool sel_pass, float pe_beam, bool swtrig,
                 float contained_frac, float associated_frac) {
                return in_fid && nslices == 1 && sel_pass && pe_beam > 20 &&
                       contained_frac >= 0.7 && associated_frac >= 0.5 &&
                       (st == SampleOrigin::kMonteCarlo ? swtrig : true);
            },
            {"in_reco_fiducial", "num_slices", "selection_pass", "optical_filter_pe_beam",
             "software_trigger", "slice_contained_fraction", "slice_cluster_fraction"});

        return next_ ? next_->process(quality_df, st) : quality_df;
    }
};

}

#endif
