#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include "PluginRegistry.h"
#include "AnalysisDefinition.h"
#include "Logger.h"
#include "BinningDefinition.h"
#include "DynamicBinning.h"
#include "IAnalysisPlugin.h"

namespace analysis {

class VariablesPlugin : public IAnalysisPlugin {
  public:
    VariablesPlugin(const PluginArgs &args, AnalysisDataLoader *)
        : config_(args.analysis_configs) {}

    void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &) override {
        log::info("VariablesPlugin::onInitialisation", "Defining variables...");
        if (!config_.contains("variables"))
            log::fatal("VariablesPlugin::onInitialisation", "no variables configured");

        for (auto const &var_cfg : config_.at("variables")) {

            auto name = var_cfg.at("name").get<std::string>();
            auto branch = var_cfg.at("branch").get<std::string>();
            auto label = var_cfg.at("label").get<std::string>();
            auto strat = var_cfg.at("stratum").get<std::string>();

            const auto &bins_cfg = var_cfg.at("bins");
            if (bins_cfg.is_string() && bins_cfg.get<std::string>() == "dynamic") {
                BinningDefinition placeholder_bins(
                    {-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()}, branch, label,
                    {}, strat);
                def.addVariable(name, branch, label, placeholder_bins, strat, true, false);
            } else if (bins_cfg.is_object() && bins_cfg.contains("mode") && bins_cfg.at("mode") == "dynamic") {
                double domain_min = bins_cfg.value("min", -std::numeric_limits<double>::infinity());
                double domain_max = bins_cfg.value("max", std::numeric_limits<double>::infinity());
                bool include_oob_bins = bins_cfg.value("include_oob_bins", true);
                std::string strat_mode = bins_cfg.value("strategy", std::string("equal_weight"));
                static const std::unordered_map<std::string, DynamicBinningStrategy> strategy_map = {
                    {"equal_weight", DynamicBinningStrategy::EqualWeight},
                    {"uniform_width", DynamicBinningStrategy::UniformWidth},
                    {"bayesian_blocks", DynamicBinningStrategy::BayesianBlocks}};
                DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight;
                auto it = strategy_map.find(strat_mode);
                if (it != strategy_map.end()) {
                    strategy = it->second;
                }
                BinningDefinition placeholder_bins({domain_min, domain_max}, branch, label, {}, strat);
                def.addVariable(name, branch, label, placeholder_bins, strat, true, include_oob_bins, strategy);
            } else {
                std::vector<double> edges;
                if (bins_cfg.is_array()) {
                    edges = bins_cfg.get<std::vector<double>>();
                } else {
                    int n = bins_cfg.at("n").get<int>();
                    double min = bins_cfg.at("min").get<double>();
                    double max = bins_cfg.at("max").get<double>();
                    for (int i = 0; i <= n; ++i)
                        edges.push_back(min + (max - min) * i / n);
                }
                BinningDefinition bins(edges, branch, label, {}, strat);
                def.addVariable(name, branch, label, bins, strat);
            }

            if (var_cfg.contains("regions")) {
                for (auto const &region_name : var_cfg.at("regions")) {
                    def.addVariableToRegion(region_name.get<std::string>(), name);
                }
            } else {
                // No default region assignment: skip this variable and warn so the
                // configuration can be fixed explicitly.
                log::warn("VariablesPlugin::onInitialisation",
                          "Variable '{}' has no 'regions' field and will not be attached to any regions",
                          name);
            }
        }
    }

    void onFinalisation(const AnalysisResult &) override {}

  private:
    nlohmann::json config_;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IAnalysisPlugin, analysis::AnalysisDataLoader,
                         "VariablesPlugin", analysis::VariablesPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const analysis::PluginArgs &args) {
    return new analysis::VariablesPlugin(args, nullptr);
}
#endif
