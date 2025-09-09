#ifndef STRATIFIER_REGISTRY_H
#define STRATIFIER_REGISTRY_H

#include <algorithm>
#include <atomic>
#include <cctype>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "ROOT/RVec.hxx"
#include "RtypesCore.h"
#include "TColor.h"

#include "AnalysisKey.h"
#include "Logger.h"

namespace analysis {

static std::atomic<int> other_log(0);

struct StratumProperties {
  int internal_key;
  std::string plain_name;
  std::string tex_label;
  Color_t fill_colour;
  int fill_style;
};

enum class StratifierType { kUnknown, kScalar, kVector };

using VectorFilterPredicate = std::function<bool(const ROOT::RVec<int> &, int)>;

class StratifierRegistry {
private:
  struct SchemeDefinition {
    std::map<int, StratumProperties> strata;
    StratifierType type;
    VectorFilterPredicate predicate = nullptr;
  };

public:
  StratifierRegistry() {
    this->addInclusiveScheme();

    this->addExclusiveScheme();

    this->addBacktrackedPDGScheme();

    this->addBlipPDGScheme();

    this->addBlipProcessCodeScheme();

    this->addChannelDefinitionScheme();

    signal_channel_groups_ = {
        {"inclusive_strange_channels", {10, 11}},
        {"exclusive_strange_channels",
         {50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61}}};
    log::info("StratifierRegistry::StratifierRegistry",
              "Registry initialised successfully.");
  }

  const StratumProperties &getStratumProperties(const std::string &scheme_name,
                                                int key) const {
    auto scheme_it = scheme_definitions_.find(scheme_name);
    if (scheme_it == scheme_definitions_.end())
      log::fatal("StratifierRegistry::getStratumProperties",
                 "Scheme not found: " + scheme_name);

    auto stratum_it = scheme_it->second.strata.find(key);
    if (stratum_it == scheme_it->second.strata.end())
      log::fatal("StratifierRegistry::getStratumProperties",
                 "Stratum not found in scheme '" + scheme_name +
                     "': " + std::to_string(key));

    return stratum_it->second;
  }

  int findStratumKeyByName(const std::string &scheme_name,
                           const std::string &stratum_name) const {
    auto scheme_it = scheme_definitions_.find(scheme_name);
    if (scheme_it != scheme_definitions_.end()) {
      std::string target = stratum_name;
      std::transform(target.begin(), target.end(), target.begin(),
                     [](unsigned char c) { return std::tolower(c); });
      for (const auto &[key, props] : scheme_it->second.strata) {
        std::string name = props.plain_name;
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        if (name == target) {
          return key;
        }
      }
    }
    return -1;
  }

  const std::vector<int> &
  getSignalKeys(const std::string &signal_group_name) const {
    auto it = signal_channel_groups_.find(signal_group_name);
    if (it == signal_channel_groups_.end()) {
      log::fatal("StratifierRegistry::getSignalKeys",
                 "Signal group not found: " + signal_group_name);
    }
    return it->second;
  }

  std::vector<int>
  getAllStratumIntKeysForScheme(const std::string &scheme_name) const {
    auto scheme_it = scheme_definitions_.find(scheme_name);
    if (scheme_it == scheme_definitions_.end())
      log::fatal("StratifierRegistry::getAllStratumIntKeysForScheme",
                 "Scheme not found: " + scheme_name);

    std::vector<int> keys;
    keys.reserve(scheme_it->second.strata.size());
    for (const auto &[key_int, _] : scheme_it->second.strata) {
      keys.push_back(key_int);
    }
    return keys;
  }

  std::vector<StratumKey>
  getAllStratumKeysForScheme(const std::string &scheme_name) const {
    auto int_keys = getAllStratumIntKeysForScheme(scheme_name);
    std::vector<StratumKey> keys;
    keys.reserve(int_keys.size());
    for (int key_int : int_keys) {
      keys.emplace_back(std::to_string(key_int));
    }
    return keys;
  }

  std::vector<std::string> getRegisteredSchemeNames() const {
    std::vector<std::string> names;
    names.reserve(scheme_definitions_.size());
    for (const auto &pair : scheme_definitions_) {
      names.push_back(pair.first);
    }
    return names;
  }

  StratifierType findSchemeType(const StratifierKey &key) const {
    auto it = scheme_definitions_.find(key.str());
    if (it != scheme_definitions_.end()) {
      return it->second.type;
    }
    return StratifierType::kUnknown;
  }

  VectorFilterPredicate findPredicate(const StratifierKey &key) const {
    auto it = scheme_definitions_.find(key.str());
    if (it == scheme_definitions_.end() ||
        it->second.type != StratifierType::kVector) {
      log::fatal("StratifierRegistry::findPredicate",
                 "No predicate found for vector scheme:", key.str());
    }
    return it->second.predicate;
  }

private:
  void addInclusiveScheme() {
    this->addScheme(
        "inclusive_strange_channels", StratifierType::kScalar,
        {{0, "Data", "Data", kBlack, 1001},
         {1, "External", "External", kTeal + 2, 3345},
         {2, "Dirt", "Dirt", kGray + 2, 1001},
         {10, "numu_cc_1s", R"(#nu_{#mu}CC 1s)", kSpring + 5, 1001},
         {11, "numu_cc_ms", R"(#nu_{#mu}CC Ms)", kGreen + 2, 1001},
         {20, "numu_cc_np0pi", R"(#nu_{#mu}CC Np0#pi)", kRed, 1001},
         {21, "numu_cc_0pnpi", R"(#nu_{#mu}CC 0pN#pi)", kRed - 7, 1001},
         {22, "numu_cc_npnpi", R"(#nu_{#mu}CC NpN#pi)", kOrange, 1001},
         {23, "numu_cc_other", R"(#nu_{#mu}CC Other)", kViolet, 1001},
         {30, "nue_cc", R"(#nu_{e}CC)", kMagenta, 1001},
         {31, "nc", R"(#nu_{x}NC)", kBlue, 1001},
         {98, "out_fv", "Out FV", kYellow - 7, 1001},
         {99, "other", "Other", kCyan, 1001}});
  }

  void addExclusiveScheme() {
    this->addScheme(
        "exclusive_strange_channels", StratifierType::kScalar,
        {{0, "Data", "Data", kBlack, 1001},
         {1, "External", "External", kTeal + 2, 3345},
         {2, "Dirt", "Dirt", kGray + 2, 1001},
         {30, "nue_cc", R"(#nu_{e}CC)", kGreen + 2, 1001},
         {31, "nc", R"(#nu_{x}NC)", kBlue + 1, 1001},
         {32, "numu_cc_other", R"(#nu_{#mu}CC Other)", kCyan + 2, 1001},
         {50, "numu_cc_kpm", R"(#nu_{#mu}CC K^{#pm})", kYellow + 2, 1001},
         {51, "numu_cc_k0", R"(#nu_{#mu}CC K^{0})", kOrange - 2, 1001},
         {52, "numu_cc_lambda", R"(#nu_{#mu}CC #Lambda^{0})", kOrange + 8,
          1001},
         {53, "numu_cc_sigmapm", R"(#nu_{#mu}CC #Sigma^{#pm})", kRed + 2, 1001},
         {54, "numu_cc_lambda_kpm", R"(#nu_{#mu}CC #Lambda^{0} K^{#pm})",
          kRed + 1, 1001},
         {55, "numu_cc_sigma_k0", R"(#nu_{#mu}CC #Sigma^{#pm} K^{0})", kRed - 7,
          1001},
         {56, "numu_cc_sigma_kmp", R"(#nu_{#mu}CC #Sigma^{#pm} K^{#mp})",
          kPink + 8, 1001},
         {57, "numu_cc_lambda_k0", R"(#nu_{#mu}CC #Lambda^{0} K^{0})",
          kPink + 2, 1001},
         {58, "numu_cc_kpm_kmp", R"(#nu_{#mu}CC K^{#pm} K^{#mp})", kMagenta + 2,
          1001},
         {59, "numu_cc_sigma0", R"(#nu_{#mu}CC #Sigma^{0})", kMagenta + 1,
          1001},
         {60, "numu_cc_sigma0_kpm", R"(#nu_{#mu}CC #Sigma^{0} K^{#pm})",
          kViolet + 1, 1001},
         {61, "numu_cc_other_strange", R"(#nu_{#mu}CC Other Strange)",
          kPink - 9, 1001},
         {98, "out_fv", "Out FV", kYellow - 7, 1001},
         {99, "other", "Other", kGray + 3, 1001}});
  }

  void addBacktrackedPDGScheme() {
    this->addScheme(
        "backtracked_pdg_codes", StratifierType::kVector,
        {{13, "muon", R"(#mu^{#pm})", kAzure - 4, 1001},
         {2212, "proton", "p", kOrange - 3, 1001},
         {211, "pion", R"(#pi^{#pm})", kGreen + 1, 1001},
         {22, "gamma", R"(#gamma)", kYellow - 7, 1001},
         {11, "electron", R"(e^{#pm})", kCyan - 3, 1001},
         {2112, "neutron", "n", kGray + 1, 1001},
         {321, "kaon", R"(K^{#pm})", kMagenta - 9, 1001},
         {3222, "sigma", R"(#Sigma^{#pm})", kRed - 9, 1001},
         {0, "none", "Cosmic", kGray + 2, 1001},
         {-1, "other", "Other", kBlack, 3005}},
        [](const ROOT::RVec<int> &pdg_codes, int key) {
          if (key == 0) {
            return ROOT::VecOps::Sum(pdg_codes == 0) > 0;
          }
          if (key == -1) {
            if (pdg_codes.empty())
              return false;
            const std::set<int> known_pdgs = {0,   11,   13,   22,   211,
                                              321, 2112, 2212, 3112, 3222};
            for (int code : pdg_codes) {
              if (known_pdgs.count(std::abs(code))) {
                return false;
              }
            }
            if (other_log < 5) {
              other_log++;
              std::cout << "[DEBUG: Stratifier] 'Other' event "
                           "contains PDG codes: ";
              for (int code : pdg_codes) {
                std::cout << code << " ";
              }
              std::cout << std::endl;
            }
            return true;
          }
          if (key == 3222) {
            return ROOT::VecOps::Sum(ROOT::VecOps::abs(pdg_codes) == 3222 ||
                                     ROOT::VecOps::abs(pdg_codes) == 3112) > 0;
          } else {
            return ROOT::VecOps::Sum(ROOT::VecOps::abs(pdg_codes) == key) > 0;
          }
        });
  }

  void addBlipPDGScheme() {
    this->addScheme("blip_pdg", StratifierType::kVector,
                    {{13, "muon", R"(#mu^{#pm})", kAzure - 4, 1001},
                     {2212, "proton", "p", kOrange - 3, 1001},
                     {211, "pion", R"(#pi^{#pm})", kGreen + 1, 1001},
                     {22, "gamma", R"(#gamma)", kYellow - 7, 1001},
                     {11, "electron", R"(e^{#pm})", kCyan - 3, 1001},
                     {2112, "neutron", "n", kGray + 1, 1001},
                     {321, "kaon", R"(K^{#pm})", kMagenta - 9, 1001},
                     {0, "none", "Cosmic", kGray + 2, 1001},
                     {-1, "other", "Other", kBlack, 3005}},
                    [](const ROOT::RVec<int> &pdg_codes, int key) {
                      if (key == 0) {
                        return ROOT::VecOps::Sum(pdg_codes == 0) > 0;
                      }
                      if (key == -1) {
                        if (pdg_codes.empty())
                          return false;
                        const std::set<int> known_pdgs = {0,   11,  13,   22,
                                                          211, 321, 2112, 2212};
                        for (int code : pdg_codes) {
                          if (known_pdgs.count(std::abs(code))) {
                            return false;
                          }
                        }
                        if (other_log < 5) {
                          other_log++;
                          std::cout << "[DEBUG: Stratifier] 'Other' "
                                       "blip contains PDG codes: ";
                          for (int code : pdg_codes) {
                            std::cout << code << " ";
                          }
                          std::cout << std::endl;
                        }
                        return true;
                      }
                      return ROOT::VecOps::Sum(ROOT::VecOps::abs(pdg_codes) ==
                                               key) > 0;
                    });
  }

  void addBlipProcessCodeScheme() {
    this->addScheme(
        "blip_process_code", StratifierType::kVector,
        {{1, "muon_capture", R"(#mu capture)", kAzure - 4, 1001},
         {2, "neutron_capture", R"(n capture)", kGreen + 2, 1001},
         {3, "neutron_inelastic", R"(n inelastic)", kMagenta - 9, 1001},
         {4, "gamma", R"(#gamma)", kYellow - 7, 1001},
         {5, "electron", R"(e processes)", kCyan - 3, 1001},
         {6, "muon", R"(#mu processes)", kBlue, 1001},
         {7, "hadron", R"(hadron ion.)", kOrange - 3, 1001},
         {0, "none", "Cosmic", kGray + 2, 1001},
         {-1, "other", "Other", kBlack, 3005}},
        [](const ROOT::RVec<int> &proc_codes, int key) {
          if (key == 0) {
            return ROOT::VecOps::Sum(proc_codes == 0) > 0;
          }
          if (key == -1) {
            if (proc_codes.empty())
              return false;
            const std::set<int> known_codes = {0, 1, 2, 3, 4, 5, 6, 7};
            for (int code : proc_codes) {
              if (known_codes.count(code)) {
                return false;
              }
            }
            if (other_log < 5) {
              other_log++;
              std::cout << "[DEBUG: Stratifier] 'Other' blip "
                           "contains process codes: ";
              for (int code : proc_codes) {
                std::cout << code << " ";
              }
              std::cout << std::endl;
            }
            return true;
          }
          return ROOT::VecOps::Sum(proc_codes == key) > 0;
        });
  }

  void addChannelDefinitionScheme() {
    this->addScheme(
        "channel_definitions", StratifierType::kScalar,
        {{0, "data", "Data", kBlack, 1001},
         {1, "external", "Cosmic", kTeal + 2, 3345},
         {2, "out_fv", "Out FV", kYellow - 7, 1001},
         {10, "numu_cc_np0pi", R"(#nu_{#mu}CC Np0pi)", kRed, 1001},
         {11, "numu_cc_0pnpi", R"(#nu_{#mu}CC 0p1#pi^{#pm})", kRed - 7, 1001},
         {12, "numu_cc_pi0gg", R"(#nu_{#mu}CC #pi^{0}/#gamma#gamma)", kOrange, 1001},
         {13, "numu_cc_npnpi", R"(#nu_{#mu}CC multi-#pi^{#pm})", kViolet, 1001},
         {14, "nc", R"(#nu_{x}NC)", kBlue, 1001},
         {15, "numu_cc_1s", R"(#nu_{#mu}CC 1s)", kSpring + 5, 1001},
         {16, "numu_cc_ms", R"(#nu_{#mu}CC Ms)", kGreen + 2, 1001},
         {17, "nue_cc", R"(#nu_{e}CC)", kMagenta, 1001},
         {18, "numu_cc_other", R"(#nu_{#mu}CC Other)", kCyan + 2, 1001},
         {99, "other", "Other", kCyan, 1001}});
  }

  void addScheme(const std::string &name, StratifierType type,
                 const std::vector<StratumProperties> &strata,
                 VectorFilterPredicate predicate = nullptr) {
    SchemeDefinition definition;
    definition.type = type;
    definition.predicate = predicate;
    for (const auto &props : strata) {
      definition.strata[props.internal_key] = props;
    }
    scheme_definitions_[name] = std::move(definition);
  }

  std::map<std::string, SchemeDefinition> scheme_definitions_;

  std::map<std::string, std::vector<int>> signal_channel_groups_;
};

} // namespace analysis

#endif
