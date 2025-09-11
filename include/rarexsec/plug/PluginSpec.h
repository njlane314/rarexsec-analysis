#pragma once
#include <string>
#include <vector>
#include <functional>

#include <rarexsec/plug/PluginArgs.h>

namespace analysis {

struct PluginSpec {
  std::string id;     // plugin name or path to .so
  PluginArgs  args{}; // structured args for that plugin
};

using PluginSpecList = std::vector<PluginSpec>;

// Deep-merge plugin argument structures. Each JSON blob is merged
// individually, preserving the behaviour that configuration blocks in later
// specifications override or extend earlier ones.
inline PluginArgs deepMerge(PluginArgs lhs, const PluginArgs& rhs) {
  std::function<nlohmann::json(nlohmann::json, const nlohmann::json&)> merge =
      [&](nlohmann::json l, const nlohmann::json& r) -> nlohmann::json {
        if (!l.is_object() || !r.is_object()) return r;
        for (auto it = r.begin(); it != r.end(); ++it) {
          const auto& k = it.key();
          if (l.contains(k) && l[k].is_object() && it.value().is_object()) {
            l[k] = merge(l[k], it.value());
          } else {
            l[k] = it.value();
          }
        }
        return l;
      };

  lhs.analysis_configs = merge(lhs.analysis_configs, rhs.analysis_configs);
  lhs.plot_configs     = merge(lhs.plot_configs, rhs.plot_configs);
  lhs.systematics_configs = merge(lhs.systematics_configs, rhs.systematics_configs);
  return lhs;
}

} // namespace analysis
