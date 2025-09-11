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
            .Define("reco_nu_vtx_sce_z", "reco_neutrino_vertex_sce_z")
            .Define("bnbdata",
                    [st]() { return st == SampleOrigin::kData ? 1 : 0; })
            .Define("extdata",
                    [st]() { return st == SampleOrigin::kExternal ? 1 : 0; });

    auto presel_df = proc_df.Define(
        "numu_presel",
        "nslice == 1 && ((_opfilter_pe_beam > 0 && _opfilter_pe_veto < 20) || "
        "bnbdata == 1 || extdata == 1) && "
        "(reco_nu_vtx_sce_z < 675 || reco_nu_vtx_sce_z > 775) && "
        "topological_score > 0.06");

    return next_ ? next_->process(presel_df, st) : presel_df;
  }
};

} // namespace analysis

#endif
