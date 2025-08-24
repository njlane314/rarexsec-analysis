#ifndef REGIONS_PLUGIN_H
#define REGIONS_PLUGIN_H

#include <nlohmann/json.hpp>

#include "AnalysisDefinition.h"
#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"

namespace analysis {

class RegionsPlugin : public IAnalysisPlugin {
public:
  explicit RegionsPlugin(const nlohmann::json &cfg) : config_(cfg) {}

  void onInitialisation(AnalysisDefinition &def,
                        const SelectionRegistry &) override {
    log::info("RegionsPlugin", "Defining regions...");
    if (!config_.contains("regions"))
      return;

    for (auto const &region_cfg : config_.at("regions")) {

      auto region_key = region_cfg.at("region_key").get<std::string>();
      auto label = region_cfg.at("label").get<std::string>();

      if (region_cfg.contains("selection_rule")) {
        auto rule_key = region_cfg.at("selection_rule").get<std::string>();
        def.addRegion(region_key, label, rule_key);
      } else if (region_cfg.contains("expression")) {
        auto expr = region_cfg.at("expression").get<std::string>();
        def.addRegionExpr(region_key, label, expr);
      } else {
        log::fatal("RegionsPlugin",
                   "each region must have either selection_rule or expression");
      }
    }
  }
  void onPreSampleProcessing(const SampleKey &, const RegionKey &,
                             const RunConfig &) override {}
  void onPostSampleProcessing(const SampleKey &, const RegionKey &,
                              const AnalysisRegionMap &) override {}
  void onFinalisation(const AnalysisRegionMap &) override {}

private:
  nlohmann::json config_;
};

}

#endif