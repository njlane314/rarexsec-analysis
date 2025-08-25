#ifndef TRUTH_CHANNEL_PROCESSOR_H
#define TRUTH_CHANNEL_PROCESSOR_H

#include <cmath>
#include <iostream>
#include <map>
#include <mutex>

#include "IEventProcessor.h"

namespace analysis {

class TruthChannelProcessor : public IEventProcessor {
  public:
    explicit TruthChannelProcessor() = default;

    ROOT::RDF::RNode process(ROOT::RDF::RNode df,
                             SampleOrigin st) const override {
        if (st != SampleOrigin::kMonteCarlo) {
            auto mode_df = df.Define("genie_int_mode", []() { return -1; });

            auto incl_df = mode_df.Define("incl_channel", [c = st]() {
                return c == SampleOrigin::kData ? 0 : 1;
            });

            auto incl_alias_df =
                incl_df.Define("inclusive_strange_channels", "incl_channel");

            auto excl_df = incl_alias_df.Define("excl_channel", [c = st]() {
                return c == SampleOrigin::kData ? 0 : 1;
            });

            auto excl_alias_df =
                excl_df.Define("exclusive_strange_channels", "excl_channel");

            return next_ ? next_->process(excl_alias_df, st) : excl_alias_df;
        }

        auto fid_df =
            df.Define("in_fiducial",
                      "(neutrino_vertex_x > 5 && neutrino_vertex_x < 251) &&"
                      "(neutrino_vertex_y > -110 && neutrino_vertex_y < 110) &&"
                      "(neutrino_vertex_z > 20 && neutrino_vertex_z < 986)");

        auto strange_df = fid_df.Define(
            "mc_n_strange",
            "count_kaon_plus + count_kaon_minus + count_kaon_zero +"
            " count_lambda + count_sigma_plus + count_sigma_zero + "
            "count_sigma_minus");

        auto pion_df =
            strange_df.Define("mc_n_pion", "count_pi_plus + count_pi_minus");

        auto proton_df = pion_df.Define("mc_n_proton", "count_proton");

        auto mode_df = proton_df.Define(
            "genie_int_mode",
            [](int mode) {
                struct ModeCounter {
                    std::map<int, long long> counts;
                    std::mutex mtx;
                    ~ModeCounter() {
                        std::cout
                            << "[DEBUG] GENIE interaction mode frequencies:\n";
                        for (const auto &kv : counts) {
                            std::cout << "  mode " << kv.first << ": "
                                      << kv.second << std::endl;
                        }
                    }
                };
                static ModeCounter counter;
                {
                    std::lock_guard<std::mutex> lock(counter.mtx);
                    counter.counts[mode]++;
                    if (counter.counts[mode] == 1 && mode != 0 && mode != 1 &&
                        mode != 2 && mode != 3 && mode != 10) {
                        std::cout
                            << "[DEBUG] Uncategorised GENIE mode: " << mode
                            << std::endl;
                    }
                }
                switch (mode) {
                case 0:
                    return 0;
                case 1:
                    return 1;
                case 2:
                    return 2;
                case 3:
                    return 3;
                case 10:
                    return 10;
                default:
                    return -1;
                }
            },
            {"interaction_mode"});

        auto incl_chan_df =
            mode_df.Define("incl_channel",
                           [](bool fv, int nu, int cc, int s, int np, int npi) {
                               if (!fv)
                                   return 98;
                               if (cc == 1)
                                   return 31;
                               if (std::abs(nu) == 12 && cc == 0)
                                   return 30;
                               if (std::abs(nu) == 14 && cc == 0) {
                                   if (s == 1)
                                       return 10;
                                   if (s > 1)
                                       return 11;
                                   if (np >= 1 && npi == 0)
                                       return 20;
                                   if (np == 0 && npi >= 1)
                                       return 21;
                                   if (np >= 1 && npi >= 1)
                                       return 22;
                                   return 23;
                               }
                               return 99;
                           },
                           {"in_fiducial", "neutrino_pdg", "interaction_ccnc",
                            "mc_n_strange", "mc_n_pion", "mc_n_proton"});

        auto incl_alias_df =
            incl_chan_df.Define("inclusive_strange_channels", "incl_channel");

        auto excl_chan_df = incl_alias_df.Define(
            "excl_channel",
            [](bool fv, int nu, int cc, int s, int kp, int km, int k0, int lam,
               int sp, int s0, int sm) {
                if (!fv)
                    return 98;
                if (cc == 1)
                    return 31;
                if (std::abs(nu) == 12 && cc == 0)
                    return 30;
                if (std::abs(nu) == 14 && cc == 0) {
                    if (s == 0)
                        return 32;
                    if ((kp == 1 || km == 1) && s == 1)
                        return 50;
                    if (k0 == 1 && s == 1)
                        return 51;
                    if (lam == 1 && s == 1)
                        return 52;
                    if ((sp == 1 || sm == 1) && s == 1)
                        return 53;
                    if (lam == 1 && (kp == 1 || km == 1) && s == 2)
                        return 54;
                    if ((sp == 1 || sm == 1) && k0 == 1 && s == 2)
                        return 55;
                    if ((sp == 1 || sm == 1) && (kp == 1 || km == 1) && s == 2)
                        return 56;
                    if (lam == 1 && k0 == 1 && s == 2)
                        return 57;
                    if (kp == 1 && km == 1 && s == 2)
                        return 58;
                    if (s0 == 1 && s == 1)
                        return 59;
                    if (s0 == 1 && kp == 1 && s == 2)
                        return 60;
                    return 61;
                }
                return 99;
            },
            {"in_fiducial", "neutrino_pdg", "interaction_ccnc", "mc_n_strange",
             "count_kaon_plus", "count_kaon_minus", "count_kaon_zero",
             "count_lambda", "count_sigma_plus", "count_sigma_zero",
             "count_sigma_minus"});

        auto excl_alias_df =
            excl_chan_df.Define("exclusive_strange_channels", "excl_channel");

        return next_ ? next_->process(excl_alias_df, st) : excl_alias_df;
    }
};

} // namespace analysis

#endif
