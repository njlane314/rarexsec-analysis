#ifndef RECONSTRUCTION_PROCESSOR_H
#define RECONSTRUCTION_PROCESSOR_H

#include "ROOT/RVec.hxx"

#include "IEventProcessor.h"

namespace analysis {

class ReconstructionProcessor : public IEventProcessor {
  public:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df,
                             SampleOrigin st) const override {
        auto fid_df = df.Define("in_reco_fiducial",
                                "reco_neutrino_vertex_sce_x > 5 && "
                                "reco_neutrino_vertex_sce_x < 251 && "
                                "reco_neutrino_vertex_sce_y > -110 && "
                                "reco_neutrino_vertex_sce_y < 110 && "
                                "reco_neutrino_vertex_sce_z > 20 && "
                                "reco_neutrino_vertex_sce_z < 986");

        auto gen2_df = fid_df.Define("n_pfps_gen2",
                                     [](const ROOT::RVec<unsigned> &gens) {
                                         return ROOT::VecOps::Sum(gens == 2u);
                                     },
                                     {"pfp_generations"});

        auto gen3_df = gen2_df.Define("n_pfps_gen3",
                                      [](const ROOT::RVec<unsigned> &gens) {
                                          return ROOT::VecOps::Sum(gens == 3u);
                                      },
                                      {"pfp_generations"});

        auto quality_df = gen3_df.Define(
            "quality_event", "in_reco_fiducial && num_slices == 1 && "
                             "selection_pass && optical_filter_pe_beam > 20");

        return next_ ? next_->process(quality_df, st) : quality_df;
    }
};

}

#endif
