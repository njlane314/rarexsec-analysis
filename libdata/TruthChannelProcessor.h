#ifndef TRUTH_CHANNEL_PROCESSOR_H
#define TRUTH_CHANNEL_PROCESSOR_H

#include <cmath>
#include "IEventProcessor.h"

namespace analysis {

class TruthChannelProcessor : public IEventProcessor {
public:
    explicit TruthChannelProcessor() = default;

    ROOT::RDF::RNode process(ROOT::RDF::RNode df, SampleType st) const override {
        if (st != SampleType::kMonteCarlo) {
            auto incl_df = df.Define(
                "incl_channel",
                [c = st]() { return c == SampleType::kData ? 0 : 1; }
            );

            auto excl_df = incl_df.Define(
                "excl_channel",
                [c = st]() { return c == SampleType::kData ? 0 : 1; }
            );

            return next_ ? next_->process(excl_df, st) : excl_df;
        }

        auto fid_df = df.Define(
            "in_fiducial",
            "(neutrino_vertex_x > 5 && neutrino_vertex_x < 251) &&"
            "(neutrino_vertex_y > -110 && neutrino_vertex_y < 110) &&"
            "(neutrino_vertex_z > 20 && neutrino_vertex_z < 986)"
        );

        auto strange_df = fid_df.Define(
            "mc_n_strange",
            "count_kaon_plus + count_kaon_minus + count_kaon_zero +"
            " count_lambda + count_sigma_plus + count_sigma_zero + count_sigma_minus"
        );

        auto pion_df = strange_df.Define(
            "mc_n_pion",
            "count_pi_plus + count_pi_minus"
        );

        auto proton_df = pion_df.Define(
            "mc_n_proton",
            "count_proton"
        );

        auto incl_chan_df = proton_df.Define(
            "incl_channel",
            [](bool fv, int nu, int cc, int s, int np, int npi) {
                if (!fv) return 98;
                if (cc == 1) return 31;
                if (std::abs(nu) == 12 && cc == 0) return 30;
                if (std::abs(nu) == 14 && cc == 0) {
                    if (s == 1) return 10;
                    if (s > 1) return 11;
                    if (np >= 1 && npi == 0) return 20;
                    if (np == 0 && npi >= 1) return 21;
                    if (np >= 1 && npi >= 1) return 22;
                    return 23;
                }
                return 99;
            },
            {"in_fiducial", "neutrino_pdg", "interaction_ccnc",
             "mc_n_strange", "mc_n_pion", "mc_n_proton"}
        );

        auto excl_chan_df = incl_chan_df.Define(
            "excl_channel",
            [](bool fv, int nu, int cc, int s,
               int kp, int km, int k0,
               int lam, int sp, int s0, int sm) {
                if (!fv) return 98;
                if (cc == 1) return 31;
                if (std::abs(nu) == 12 && cc == 0) return 30;
                if (std::abs(nu) == 14 && cc == 0) {
                    if (s == 0) return 32;
                    if ((kp == 1 || km == 1) && s == 1) return 50;
                    if (k0 == 1 && s == 1) return 51;
                    if (lam == 1 && s == 1) return 52;
                    if ((sp == 1 || sm == 1) && s == 1) return 53;
                    if (lam == 1 && (kp == 1 || km == 1) && s == 2) return 54;
                    if ((sp == 1 || sm == 1) && k0 == 1 && s == 2) return 55;
                    if ((sp == 1 || sm == 1) && (kp == 1 || km == 1) && s == 2) return 56;
                    if (lam == 1 && k0 == 1 && s == 2) return 57;
                    if (kp == 1 && km == 1 && s == 2) return 58;
                    if (s0 == 1 && s == 1) return 59;
                    if (s0 == 1 && kp == 1 && s == 2) return 60;
                    return 61;
                }
                return 99;
            },
            {"in_fiducial", "neutrino_pdg", "interaction_ccnc",
             "mc_n_strange", "count_kaon_plus", "count_kaon_minus",
             "count_kaon_zero", "count_lambda", "count_sigma_plus",
             "count_sigma_zero", "count_sigma_minus"}
        );

        return next_ ? next_->process(excl_chan_df, st) : excl_chan_df;
    }
};

} 

#endif // TRUTH_CHANNEL_PROCESSOR_H
