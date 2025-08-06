#ifndef STRATIFICATION_REGISTRY_H
#define STRATIFICATION_REGISTRY_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include "RtypesCore.h"
#include "TColor.h"
#include "Logger.h"

namespace analysis {

struct StratumProperties {
    int             internal_key;
    std::string     plain_name;
    std::string     tex_label;
    Color_t         fill_colour;
    int             fill_style;
};

class StratificationRegistry {
public:
    StratificationRegistry() {
        this->registerSchemes(scalar_schemes_);
        this->registerSchemes(vector_schemes_);
        signal_definitions_ = {
            {"inclusive_strange_channels", {10, 11}},
            {"exclusive_strange_channels", {
                50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61
            }}
        };
        log::info("StratificationRegistry", "Registry initialised successfully.");
    }

    const StratumProperties& getStratum(
        const std::string& scheme,
        int key) const
    {
        auto sit = stratification_schemes_.find(scheme);
        if (sit == stratification_schemes_.end())
            log::fatal("StratificationRegistry", "Scheme not found: "+scheme);
        auto it = sit->second.find(key);
        if (it == sit->second.end())
            log::fatal("StratificationRegistry", "Stratum not found: "+std::to_string(key));
        return it->second;
    }

    const std::vector<int>& getSignalKeys(
        const std::string& scheme) const
    {
        auto it = signal_definitions_.find(scheme);
        if (it == signal_definitions_.end())
            log::fatal("StratificationRegistry", "Signal scheme not found: "+scheme);
        return it->second;
    }

    std::vector<int> getStratumKeys(
        const std::string& scheme) const
    {
        auto sit = stratification_schemes_.find(scheme);
        if (sit == stratification_schemes_.end())
            log::fatal("StratificationRegistry", "Scheme not found: "+scheme);
        std::vector<int> keys;
        keys.reserve(sit->second.size());
        for (auto const& p : sit->second)
            keys.push_back(p.first);
        return keys;
    }

private:
    template<typename Seq>
    void registerSchemes(const Seq& schemes) {
        for (auto const& [scheme, strata] : schemes) {
            for (auto const& props : strata) {
                stratification_schemes_[scheme][props.internal_key] = props;
            }
        }
    }

    inline static const std::vector<
        std::pair<std::string, std::vector<StratumProperties>>>
    scalar_schemes_ = {
        {"inclusive_strange_channels",{
            {0, "Data", "Data", kBlack, 1001},
            {1, "External", "External", kGray, 3004},
            {2, "Dirt", "Dirt", kGray+2, 1001},
            {10, "numu_cc_1s", R"(#nu_{#mu}CC 1s)", kSpring+5, 1001},
            {11, "numu_cc_ms", R"(#nu_{#mu}CC Ms)", kGreen+2, 1001},
            {20, "numu_cc_np0pi", R"(#nu_{#mu}CC Np0#pi)", kRed, 1001},
            {21, "numu_cc_0pnpi", R"(#nu_{#mu}CC 0pN#pi)", kRed-7, 1001},
            {22, "numu_cc_npnpi", R"(#nu_{#mu}CC NpN#pi)", kOrange, 1001},
            {23, "numu_cc_other", R"(#nu_{#mu}CC Other)", kViolet, 1001},
            {30, "nue_cc", R"(#nu_{e}CC)", kMagenta, 1001},
            {31, "nc", R"(#nu_{x}NC)", kBlue, 1001},
            {98, "out_fv", "Out FV", kGray+1, 3004},
            {99, "other", "Other", kCyan, 1001}
        }},
        {"exclusive_strange_channels",{
            {0, "Data", "Data", kBlack, 1001},
            {1, "External", "External", kGray, 3004},
            {2, "Dirt", "Dirt", kGray+2, 1001},
            {30, "nue_cc", R"(#nu_{e}CC)", kGreen+2, 1001},
            {31, "nc", R"(#nu_{x}NC)", kBlue+1, 1001},
            {32, "numu_cc_other", R"(#nu_{#mu}CC Other)", kCyan+2, 1001},
            {50, "numu_cc_kpm", R"(#nu_{#mu}CC K^{#pm})", kYellow+2, 1001},
            {51, "numu_cc_k0", R"(#nu_{#mu}CC K^{0})", kOrange-2, 1001},
            {52, "numu_cc_lambda", R"(#nu_{#mu}CC #Lambda^{0})", kOrange+8, 1001},
            {53, "numu_cc_sigmapm", R"(#nu_{#mu}CC #Sigma^{#pm})", kRed+2, 1001},
            {54, "numu_cc_lambda_kpm", R"(#nu_{#mu}CC #Lambda^{0} K^{#pm})", kRed+1, 1001},
            {55, "numu_cc_sigma_k0", R"(#nu_{#mu}CC #Sigma^{#pm} K^{0})", kRed-7, 1001},
            {56, "numu_cc_sigma_kmp", R"(#nu_{#mu}CC #Sigma^{#pm} K^{#mp})", kPink+8, 1001},
            {57, "numu_cc_lambda_k0", R"(#nu_{#mu}CC #Lambda^{0} K^{0})", kPink+2, 1001},
            {58, "numu_cc_kpm_kmp", R"(#nu_{#mu}CC K^{#pm} K^{#mp})", kMagenta+2, 1001},
            {59, "numu_cc_sigma0", R"(#nu_{#mu}CC #Sigma^{0})", kMagenta+1, 1001},
            {60, "numu_cc_sigma0_kpm", R"(#nu_{#mu}CC #Sigma^{0} K^{#pm})", kViolet+1, 1001},
            {61, "numu_cc_other_strange", R"(#nu_{#mu}CC Other Strange)", kPink-9, 1001},
            {98, "out_fv", "Out FV", kGray+1, 3004},
            {99, "other", "Other", kGray+3, 1001}
        }}
    };

    inline static const std::vector<
        std::pair<std::string, std::vector<StratumProperties>>>
    vector_schemes_ = {
        {"backtracked_pdg_codes",{
            {13, "muon", R"(#mu^{#pm})", kAzure+2, 1001},
            {2212, "proton", "p", kOrange+1, 1001},
            {211, "pion", R"(#pi^{#pm})", kTeal+1, 1001},
            {321, "kaon", R"(K^{#pm})", kPink+1, 1001},
            {3224, "sigma", R"(#Sigma^{#pm})", kSpring-5, 1001},
            {22, "gamma", R"(#gamma)", kOrange-9, 1001},
            {11, "electron", R"(e^{#pm})", kCyan-7, 1001},
            {0, "other", "Other", kGray, 1001}
        }}
    };

    std::map<std::string,std::map<int, StratumProperties>> stratification_schemes_;
    std::map<std::string,std::vector<int>>                 signal_definitions_;
};

} 

#endif // STRATIFICATION_REGISTRY_H
