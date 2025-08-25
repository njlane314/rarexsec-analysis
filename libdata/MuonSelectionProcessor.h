#ifndef MUON_SELECTION_PROCESSOR_H
#define MUON_SELECTION_PROCESSOR_H

#include "ROOT/RVec.hxx"

#include "IEventProcessor.h"

namespace analysis {

class MuonSelectionProcessor : public IEventProcessor {
  public:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df,
                             SampleOrigin st) const override {
        if (!df.HasColumn("track_trunk_rr_dedx_u")) {
            return next_ ? next_->process(df, st) : df;
        }

        auto avg_df =
            df.Define("trk_rr_dedx_avg",
                      [](const ROOT::RVec<float> &u, const ROOT::RVec<float> &v,
                         const ROOT::RVec<float> &y) {
                          ROOT::RVec<float> avg(u.size());
                          for (size_t i = 0; i < u.size(); ++i) {
                              int count = 0;
                              float sum = 0;
                              if (u[i] > 0) {
                                  sum += u[i];
                                  ++count;
                              }
                              if (v[i] > 0) {
                                  sum += v[i];
                                  ++count;
                              }
                              if (y[i] > 0) {
                                  sum += y[i];
                                  ++count;
                              }
                              avg[i] = count ? sum / count : -1;
                          }
                          return avg;
                      },
                      {"track_trunk_rr_dedx_u", "track_trunk_rr_dedx_v",
                       "track_trunk_rr_dedx_y"});

        auto mask_df = avg_df.Define(
            "muon_mask",
            [](const ROOT::RVec<float> &scores,
               const ROOT::RVec<float> &lengths, const ROOT::RVec<float> &dists,
               const ROOT::RVec<float> &avg) {
                static_cast<void>(dists);
                ROOT::RVec<bool> mask(scores.size());
                for (size_t i = 0; i < scores.size(); ++i) {
                    mask[i] =
                        (scores[i] > 0.3f && lengths[i] > 5 && avg[i] < 3);
                }
                return mask;
            },
            {"track_shower_scores", "track_length", "track_distance_to_vertex",
             "trk_rr_dedx_avg"});

        auto count_df =
            mask_df.Define("n_muons", "ROOT::VecOps::Sum(muon_mask)");

        auto has_df = count_df.Define("has_muon", "n_muons > 0");

        return next_ ? next_->process(has_df, st) : has_df;
    }
};

} // namespace analysis

#endif
