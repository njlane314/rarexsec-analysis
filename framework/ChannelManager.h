#ifndef CHANNEL_MANAGER_H
#define CHANNEL_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include "RtypesCore.h"
#include "TColor.h"

namespace AnalysisFramework {

class ChannelManager {
public:
    struct Particle {
        std::string name;
        std::string tex_name;
        Color_t color;
    };

    struct Channel {
        int category_code;
        std::string name;
        std::string tex_name;
        Color_t color;
        int fill_style;
    };

    ChannelManager() {
        registerChannels();
        registerParticles();
        registerSignalDefs();
    }

    const Channel& getChannel(const std::string& scheme, int id) const {
        return channel_schemes_.at(scheme).at(id);
    }

    const Particle& getParticle(int pdg) const {
        return particle_definitions_.at(pdg);
    }

    const std::vector<int>& getSignalChannelKeys(const std::string& scheme) const {
        return signal_definitions_.at(scheme);
    }

    std::vector<int> getChannelKeys(const std::string& scheme) const {
        std::vector<int> keys;
        for (const auto& pair : channel_schemes_.at(scheme)) {
            keys.push_back(pair.first);
        }
        return keys;
    }

    std::vector<int> getParticleKeys() const {
        std::vector<int> keys;
        for (const auto& pair : particle_definitions_) {
            keys.push_back(pair.first);
        }
        return keys;
    }

private:
    void registerChannels() {
        channel_schemes_["inclusive_strange_channels"] = {
            {0,  {0,  "Data",                    "Data",                      kBlack,      1001}},
            {1,  {1,  "External",                "External",                  kGray,       3004}},
            {2,  {2,  "Dirt",                    "Dirt",                      kGray + 2,   1001}},
            {10, {10, "numu_cc_1s",              R"(#nu_{#mu}CC 1s)",         kSpring + 5, 1001}},
            {11, {11, "numu_cc_ms",              R"(#nu_{#mu}CC Ms)",         kGreen + 2,  1001}},
            {20, {20, "numu_cc_np0pi",           R"(#nu_{#mu}CC Np0#pi)",     kRed,        1001}},
            {21, {21, "numu_cc_0pnpi",           R"(#nu_{#mu}CC 0pN#pi)",     kRed - 7,    1001}},
            {22, {22, "numu_cc_npnpi",           R"(#nu_{#mu}CC NpN#pi)",     kOrange,     1001}},
            {23, {23, "numu_cc_other",           R"(#nu_{#mu}CC Other)",      kViolet,     1001}},
            {30, {30, "nue_cc",                  R"(#nu_{e}CC)",              kMagenta,    1001}},
            {31, {31, "nc",                      R"(#nu_{x}NC)",              kBlue,       1001}},
            {98, {98, "out_fv",                  "Out FV",                    kGray + 1,   3004}},
            {99, {99, "other",                   "Other",                     kCyan,       1001}}
        };

        channel_schemes_["exclusive_strange_channels"] = {
            {0,  {0,  "Data",                    "Data",                                        kBlack,         1001}},
            {1,  {1,  "External",                "External",                                    kGray,          3004}},
            {2,  {2,  "Dirt",                    "Dirt",                                        kGray + 2,      1001}},
            {30, {30, "nue_cc",                  R"(#nu_{e}CC)",                                kGreen + 2,     1001}},
            {31, {31, "nc",                      R"(#nu_{x}NC)",                                kBlue + 1,      1001}},
            {32, {32, "numu_cc_other",           R"(#nu_{#mu}CC Other)",                        kCyan + 2,      1001}},
            {50, {50, "numu_cc_kpm",             R"(#nu_{#mu}CC K^{#pm})",                      kYellow + 2,    1001}},
            {51, {51, "numu_cc_k0",              R"(#nu_{#mu}CC K^{0})",                        kOrange - 2,    1001}},
            {52, {52, "numu_cc_lambda",          R"(#nu_{#mu}CC #Lambda^{0})",                  kOrange + 8,    1001}},
            {53, {53, "numu_cc_sigmapm",         R"(#nu_{#mu}CC #Sigma^{#pm})",                 kRed + 2,       1001}},
            {54, {54, "numu_cc_lambda_kpm",      R"(#nu_{#mu}CC #Lambda^{0} K^{#pm})",          kRed + 1,       1001}},
            {55, {55, "numu_cc_sigma_k0",        R"(#nu_{#mu}CC #Sigma^{#pm} K^{0})",           kRed - 7,       1001}},
            {56, {56, "numu_cc_sigma_kmp",       R"(#nu_{#mu}CC #Sigma^{#pm} K^{#mp})",         kPink + 8,      1001}},
            {57, {57, "numu_cc_lambda_k0",       R"(#nu_{#mu}CC #Lambda^{0} K^{0})",            kPink + 2,      1001}},
            {58, {58, "numu_cc_kpm_kmp",         R"(#nu_{#mu}CC K^{#pm} K^{#mp})",              kMagenta + 2,   1001}},
            {59, {59, "numu_cc_sigma0",          R"(#nu_{#mu}CC #Sigma^{0})",                   kMagenta + 1,   1001}},
            {60, {60, "numu_cc_sigma0_kpm",      R"(#nu_{#mu}CC #Sigma^{0} K^{#pm})",           kViolet + 1,    1001}},
            {61, {61, "numu_cc_other_strange",   R"(#nu_{#mu}CC Other Strange)",                kPink - 9,      1001}},
            {98, {98, "out_fv",                  "Out FV",                                      kGray + 1,      3004}},
            {99, {99, "other",                   "Other",                                       kGray + 3,      1001}}
        };
    }

    void registerParticles() {
        particle_definitions_ = {
            {13,   {"muon",     R"(#mu^{#pm})",      kAzure + 2}},
            {2212, {"proton",   R"(p)",              kOrange + 1}},
            {211,  {"pion",     R"(#pi^{#pm})",      kTeal + 1}},
            {321,  {"kaon",     R"(K^{#pm})",        kPink + 1}},
            {3224, {"sigma",    R"(#Sigma^{#pm})",   kSpring - 5}},
            {22,   {"gamma",    R"(#gamma)",         kOrange - 9}},
            {11,   {"electron", R"(e^{#pm})",        kCyan - 7}},
            {0,    {"other",    "Other",             kGray}}
        };
    }

    void registerSignalDefs() {
        signal_definitions_["inclusive_strange_channels"] = {10, 11};
        signal_definitions_["exclusive_strange_channels"] = {50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61};
    }

    std::map<std::string, std::map<int, Channel>> channel_schemes_;
    std::map<int, Particle> particle_definitions_;
    std::map<std::string, std::vector<int>> signal_definitions_;
};

}
#endif