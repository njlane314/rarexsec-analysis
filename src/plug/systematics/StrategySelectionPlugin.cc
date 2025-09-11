#include <rarexsec/plug/ISystematicsPlugin.h>
#include <rarexsec/plug/PluginRegistry.h>
#include <rarexsec/syst/DetectorSystematicStrategy.h>
#include <rarexsec/syst/UniverseSystematicStrategy.h>
#include <rarexsec/syst/WeightSystematicStrategy.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace analysis {

// Plugin that constructs a selectable set of SystematicStrategy instances.
// Strategies listed in the optional "enabled" array are added to the
// processor. If no list is provided all available strategies are added.
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
    const auto &knobs = proc.knobDefinitions();
    const auto &universes = proc.universeDefinitions();
    const bool filter = !enabled_.empty();

    const std::string det_name = DetectorSystematicStrategy().getName();
    if (!filter || enabled_.count(det_name)) {
      vec.emplace_back(std::make_unique<DetectorSystematicStrategy>());
    }

    for (const auto &k : knobs) {
      if (filter && enabled_.count(k.name_) == 0)
        continue;
      vec.emplace_back(std::make_unique<WeightSystematicStrategy>(k));
    }

    for (auto u : universes) {
      if (filter && enabled_.count(u.name_) == 0)
        continue;
      auto it = universe_counts_.find(u.name_);
      if (it != universe_counts_.end())
        u.n_universes_ = it->second;
      vec.emplace_back(std::make_unique<UniverseSystematicStrategy>(u, proc.storeUniverseHists()));
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

