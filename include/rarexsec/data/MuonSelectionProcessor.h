#ifndef MUON_SELECTION_PROCESSOR_H
#define MUON_SELECTION_PROCESSOR_H

#include <cmath>

#include "ROOT/RVec.hxx"

#include <rarexsec/data/IEventProcessor.h>

namespace analysis {

class MuonSelectionProcessor : public IEventProcessor {
  public:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleOrigin st) const override {
        if (!df.HasColumn("track_trunk_rr_dedx_u")) {
            return next_ ? next_->process(df, st) : df;
        }

        auto dedx_df = this->computeAverageDedx(df);

        auto muon_mask_df = this->buildMuonMask(dedx_df);

        auto muon_features_df = this->extractMuonFeatures(muon_mask_df);

        return next_ ? next_->process(muon_features_df, st) : muon_features_df;
    }

  private:
    ROOT::RDF::RNode computeAverageDedx(ROOT::RDF::RNode df) const {
        return df.Define("trk_rr_dedx_avg",
                         [](const ROOT::RVec<float> &u, const ROOT::RVec<float> &v, const ROOT::RVec<float> &y) {
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
                         {"track_trunk_rr_dedx_u", "track_trunk_rr_dedx_v", "track_trunk_rr_dedx_y"});
    }

    ROOT::RDF::RNode buildMuonMask(ROOT::RDF::RNode df) const {
        return df.Define("muon_mask",
                         [](const ROOT::RVec<float> &scores, const ROOT::RVec<float> &lengths,
                            const ROOT::RVec<float> &dists, const ROOT::RVec<float> &avg) {
                             static_cast<void>(dists);
                             ROOT::RVec<bool> mask(scores.size());
                             for (size_t i = 0; i < scores.size(); ++i) {
                                 mask[i] = (scores[i] > 0.3f && lengths[i] > 5 && avg[i] < 3);
                             }
                             return mask;
                         },
                         {"track_shower_scores", "track_length", "track_distance_to_vertex", "trk_rr_dedx_avg"});
    }

    ROOT::RDF::RNode extractMuonFeatures(ROOT::RDF::RNode df) const {
        auto score_df = df.Define("muon_score", "ROOT::VecOps::Any(track_shower_scores > 0.3f)");

        auto length_df = score_df.Define("muon_length", "ROOT::VecOps::Any((track_shower_scores > 0.3f) && (track_length > 5))");

        auto mu_len_df = length_df.Define("muon_track_length",
                                          [](const ROOT::RVec<float> &lengths, const ROOT::RVec<bool> &mask) {
                                              ROOT::RVec<float> out;
                                              out.reserve(lengths.size());
                                              for (size_t i = 0; i < lengths.size(); ++i) {
                                                  if (mask[i])
                                                      out.push_back(lengths[i]);
                                              }
                                              return out;
                                          },
                                          {"track_length", "muon_mask"});

        auto mu_cos_df = mu_len_df.Define("muon_track_costheta",
                                          [](const ROOT::RVec<float> &theta, const ROOT::RVec<bool> &mask) {
                                              ROOT::RVec<float> out;
                                              out.reserve(theta.size());
                                              for (size_t i = 0; i < theta.size(); ++i) {
                                                  if (mask[i])
                                                      out.push_back(std::cos(theta[i]));
                                              }
                                              return out;
                                          },
                                          {"track_theta", "muon_mask"});

        auto count_df = mu_cos_df.Define("n_muons", "ROOT::VecOps::Sum(muon_mask)");

        return count_df.Define("has_muon", "n_muons > 0");
    }
};

}

#endif
