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
        log::info("RegionsPlugin::onInitialisation", "Defining regions...");
        if (!config_.contains("regions"))
            return;

        for (auto const &region_cfg : config_.at("regions")) {

            auto region_key = region_cfg.at("region_key").get<std::string>();
            auto label = region_cfg.at("label").get<std::string>();

            bool blinded = region_cfg.value("blinded", true);
            std::string beam_cfg =
                region_cfg.value("beam_config", std::string{});
            std::vector<std::string> runs =
                region_cfg.value("runs", std::vector<std::string>{});

            if (region_cfg.contains("selection_rule")) {
                auto rule_key =
                    region_cfg.at("selection_rule").get<std::string>();
                def.addRegion(region_key, label, rule_key, 0.0, blinded,
                              beam_cfg, runs);
            } else if (region_cfg.contains("expression")) {
                auto expr = region_cfg.at("expression").get<std::string>();
                def.addRegionExpr(region_key, label, expr, 0.0, blinded,
                                  beam_cfg, runs);
            } else {
                log::fatal("RegionsPlugin::onInitialisation",
                           "each region must have either selection_rule or "
                           "expression");
            }
        }
    }
    void onPreSampleProcessing(const SampleKey &, const RegionKey &,
                               const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &,
                                const RegionAnalysisMap &) override {}
    void onFinalisation(const AnalysisResult &) override {}

  private:
    nlohmann::json config_;
};

}

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createRegionsPlugin(const nlohmann::json &cfg) {
    return new analysis::RegionsPlugin(cfg);
}
#endif

