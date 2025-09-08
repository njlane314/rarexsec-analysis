#include "PluginRegistry.h"
#include "AnalysisDefinition.h"
#include "Logger.h"
#include "IAnalysisPlugin.h"

namespace analysis {

class RegionsPlugin : public IAnalysisPlugin {
  public:
    RegionsPlugin(const PluginArgs &args, AnalysisDataLoader *)
        : config_(args.analysis_configs) {}

    void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &) override {
        log::info("RegionsPlugin::onInitialisation", "Defining regions...");
        if (!config_.contains("regions"))
            log::fatal("RegionsPlugin::onInitialisation", "no regions configured");

        for (auto const &region_cfg : config_.at("regions")) {

            auto region_key = region_cfg.at("region_key").get<std::string>();
            auto label = region_cfg.at("label").get<std::string>();

            bool blinded = region_cfg.value("blinded", true);
            std::string beam_cfg = region_cfg.value("beam_config", std::string{});
            std::vector<std::string> runs = region_cfg.value("runs", std::vector<std::string>{});

            if (region_cfg.contains("selection_rule")) {
                auto rule_key = region_cfg.at("selection_rule").get<std::string>();
                def.addRegion(region_key, label, rule_key, 0.0, blinded, beam_cfg, runs);
            } else if (region_cfg.contains("expression")) {
                auto expr = region_cfg.at("expression").get<std::string>();
                def.addRegionExpr(region_key, label, expr, 0.0, blinded, beam_cfg, runs);
            } else {
                log::fatal("RegionsPlugin::onInitialisation", "each region must have either selection_rule or "
                                                              "expression");
            }
        }
    }
    void onFinalisation(const AnalysisResult &) override {}

  private:
    nlohmann::json config_;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IAnalysisPlugin, analysis::AnalysisDataLoader,
                         "RegionsPlugin", analysis::RegionsPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createRegionsPlugin(const analysis::PluginArgs &args) {
    return new analysis::RegionsPlugin(args, nullptr);
}
#endif
