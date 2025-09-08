#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace analysis {

using PluginArgs = nlohmann::json;

struct PluginSpec {
  std::string id;     // plugin name or path to .so
  PluginArgs  args{}; // free-form args for that plugin
};

using PluginSpecList = std::vector<PluginSpec>;

// Deep-merge JSON objects: rhs overrides/extends lhs
inline PluginArgs deepMerge(PluginArgs lhs, const PluginArgs& rhs) {
  if (!lhs.is_object() || !rhs.is_object()) return rhs;
  for (auto it = rhs.begin(); it != rhs.end(); ++it) {
    const auto& k = it.key();
    if (lhs.contains(k) && lhs[k].is_object() && it.value().is_object()) {
      lhs[k] = deepMerge(lhs[k], it.value());
    } else {
      lhs[k] = it.value();
    }
  }
  return lhs;
}

} // namespace analysis
