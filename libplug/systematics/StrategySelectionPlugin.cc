#include "ISystematicsPlugin.h"
#include "PluginRegistry.h"
#include "UniverseSystematicStrategy.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace analysis {

// Plugin that filters the set of existing SystematicStrategy instances.
// A list of strategy names can be provided via the "enabled" array in the
// systematics plugin configuration. Any strategies whose names are not in
// this list are removed from the processor before booking.
class StrategySelectionPlugin : public ISystematicsPlugin {
public:
  StrategySelectionPlugin(const PluginArgs &args, SystematicsProcessor *) {
    if (args.systematics_configs.contains("enabled")) {
      for (const auto &name : args.systematics_configs["enabled"]) {
        enabled_.insert(name.get<std::string>());
      }
    }

    if (args.systematics_configs.contains("universes")) {
      for (auto it = args.systematics_configs["universes"].begin();
           it != args.systematics_configs["universes"].end(); ++it) {
        universe_counts_[it.key()] = it.value().get<unsigned>();
      }
    }
  }

  void configure(SystematicsProcessor &proc) override {
    auto &vec = proc.strategies();

    if (!enabled_.empty()) {
      vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const auto &ptr) {
                  return enabled_.count(ptr->getName()) == 0;
                }),
                vec.end());
    }

    if (!universe_counts_.empty()) {
      for (auto &ptr : vec) {
        if (auto *u = dynamic_cast<UniverseSystematicStrategy *>(ptr.get())) {
          auto it = universe_counts_.find(u->getName());
          if (it != universe_counts_.end()) {
            u->setUniverseCount(it->second);
          }
        }
      }
    }
  }

private:
  std::unordered_set<std::string> enabled_;
  std::unordered_map<std::string, unsigned> universe_counts_;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::ISystematicsPlugin, analysis::SystematicsProcessor,
                         "StrategySelectionPlugin", analysis::StrategySelectionPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::ISystematicsPlugin *
createPlugin(const analysis::PluginArgs &args) {
  return new analysis::StrategySelectionPlugin(args, nullptr);
}
#endif

