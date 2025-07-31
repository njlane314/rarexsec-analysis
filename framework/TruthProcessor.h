#ifndef TRUTH_PROCESSOR_H
#define TRUTH_PROCESSOR_H

#include "ProcessingStep.h"
#include "DataTypes.h"
#include <cmath>

namespace AnalysisFramework {

class TruthProcessor : public ProcessingStep {
public:
    explicit TruthProcessor(SampleType sample_type) : sample_type_(sample_type) {}

protected:
    ROOT::RDF::RNode process(ROOT::RDF::RNode df) const override {
        if (sample_type_ != SampleType::kMonteCarlo) {
            auto d = df.Define("inclusive_strange_channels", [st = sample_type_](){
                if (st == SampleType::kData) return 0;
                if (st == SampleType::kExternal) return 1;
                return 99;
            });
            return d.Define("exclusive_strange_channels", [st = sample_type_](){
                if (st == SampleType::kData) return 0;
                if (st == SampleType::kExternal) return 1;
                return 99;
            });
        }

        auto d = df.Define("in_fv", "(neutrino_vertex_x > 5.0 && neutrino_vertex_x < 251.0 && neutrino_vertex_y > -110.0 && neutrino_vertex_y < 110.0 && neutrino_vertex_z > 20.0 && neutrino_vertex_z < 986.0)");
        d = d.Define("mc_n_strangeness", "count_kaon_plus + count_kaon_minus + count_kaon_zero + count_lambda + count_sigma_plus + count_sigma_zero + count_sigma_minus");
        d = d.Define("mc_n_pions", "count_pi_plus + count_pi_minus");
        d = d.Define("mc_n_protons", "count_proton");

        d = d.Define("inclusive_strange_channels",
            [](bool is_fid, int nu, int cc, int strange, int n_pi, int n_p) {
                if (!is_fid) return 98;
                if (cc == 1) return 31;
                if (std::abs(nu) == 12 && cc == 0) return 30;
                if (std::abs(nu) == 14 && cc == 0) {
                    if (strange == 1) return 10;
                    if (strange > 1) return 11;
                    if (n_p >= 1 && n_pi == 0) return 20;
                    if (n_p == 0 && n_pi >= 1) return 21;
                    if (n_p >= 1 && n_pi >= 1) return 22;
                    return 23;
                }
                return 99;
            }, {"in_fv", "neutrino_pdg", "interaction_ccnc", "mc_n_strangeness", "mc_n_pions", "mc_n_protons"});

        d = d.Define("exclusive_strange_channels",
            [](bool is_fid, int nu, int cc, int strange, int n_kp, int n_km, int n_k0, int n_lambda, int n_sigma_p, int n_sigma_0, int n_sigma_m) {
                if (!is_fid) return 98;
                if (cc == 1) return 31;
                if (std::abs(nu) == 12 && cc == 0) return 30;
                if (std::abs(nu) == 14 && cc == 0) {
                    if (strange == 0) return 32;
                    if ((n_kp == 1 || n_km == 1) && strange == 1) return 50;
                    if (n_k0 == 1 && strange == 1) return 51;
                    if (n_lambda == 1 && strange == 1) return 52;
                    if ((n_sigma_p == 1 || n_sigma_m == 1) && strange == 1) return 53;
                    if (n_lambda == 1 && (n_kp == 1 || n_km == 1) && strange == 2) return 54;
                    if ((n_sigma_p == 1 || n_sigma_m == 1) && n_k0 == 1 && strange == 2) return 55;
                    if ((n_sigma_p == 1 || n_sigma_m == 1) && (n_kp == 1 || n_km == 1) && strange == 2) return 56;
                    if (n_lambda == 1 && n_k0 == 1 && strange == 2) return 57;
                    if (n_kp == 1 && n_km == 1 && strange == 2) return 58;
                    if (n_sigma_0 == 1 && strange == 1) return 59;
                    if (n_sigma_0 == 1 && n_kp == 1 && strange == 2) return 60;
                    return 61;
                }
                return 99;
            }, {"in_fv", "neutrino_pdg", "interaction_ccnc", "mc_n_strangeness", "count_kaon_plus", "count_kaon_minus", "count_kaon_zero", "count_lambda", "count_sigma_plus", "count_sigma_zero", "count_sigma_minus"});

        return d;
    }

private:
    SampleType sample_type_;
};

}
#endif