#pragma once

#include <map>
#include <string>
#include <vector>

#include <rarexsec/core/RegionAnalysis.h>

namespace analysis {

namespace detail {
// Compute survival efficiencies for given keys within a scheme.
inline std::map<int, std::vector<double>>
computeEfficiencies(const std::vector<RegionAnalysis::StageCount> &counts,
                    const std::string &scheme,
                    const std::vector<int> &keys) {
  std::map<int, std::vector<double>> eff;
  if (counts.empty())
    return eff;

  auto scheme_it0 = counts[0].schemes.find(scheme);
  if (scheme_it0 == counts[0].schemes.end())
    return eff;

  std::map<int, double> initial;
  for (int key : keys) {
    auto it0 = scheme_it0->second.find(key);
    initial[key] = (it0 != scheme_it0->second.end()) ? it0->second.first : 0.0;
    eff[key].resize(counts.size(), 0.0);
  }

  for (size_t i = 0; i < counts.size(); ++i) {
    auto scheme_it = counts[i].schemes.find(scheme);
    for (int key : keys) {
      double val = 0.0;
      if (scheme_it != counts[i].schemes.end()) {
        auto it = scheme_it->second.find(key);
        if (it != scheme_it->second.end())
          val = it->second.first;
      }
      double denom = initial[key];
      eff[key][i] = denom > 0.0 ? val / denom : 0.0;
    }
  }

  return eff;
}
} // namespace detail

struct CutFlowGradient {
  std::vector<double> signal;                       // gradient for signal per stage
  std::map<int, std::vector<double>> backgrounds;   // gradients per background key
};

inline CutFlowGradient computeCutFlowGradient(
    const std::vector<RegionAnalysis::StageCount> &plus,
    const std::vector<RegionAnalysis::StageCount> &minus,
    const std::string &scheme, int signal_key,
    const std::vector<int> &background_keys) {
  CutFlowGradient grad;
  std::vector<int> keys = background_keys;
  keys.push_back(signal_key);

  auto eff_plus = detail::computeEfficiencies(plus, scheme, keys);
  auto eff_minus = detail::computeEfficiencies(minus, scheme, keys);

  size_t n = plus.size();
  grad.signal.resize(n, 0.0);
  for (size_t i = 0; i < n; ++i) {
    grad.signal[i] = (eff_plus[signal_key][i] - eff_minus[signal_key][i]) / 2.0;
  }

  for (int bk : background_keys) {
    auto &vec = grad.backgrounds[bk];
    vec.resize(n, 0.0);
    for (size_t i = 0; i < n; ++i) {
      vec[i] = (eff_plus[bk][i] - eff_minus[bk][i]) / 2.0;
    }
  }

  return grad;
}

} // namespace analysis

