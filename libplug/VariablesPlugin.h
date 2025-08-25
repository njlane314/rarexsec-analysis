#ifndef VARIABLES_PLUGIN_H
#define VARIABLES_PLUGIN_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <limits>

#include "AnalysisDefinition.h"
#include "AnalysisLogger.h"
#include "BinningDefinition.h"
#include "IAnalysisPlugin.h"

namespace analysis {

class VariablesPlugin : public IAnalysisPlugin {
public:
    explicit VariablesPlugin(const nlohmann::json& cfg)
        : config_(cfg) {}

    void onInitialisation(AnalysisDefinition& def,
                            const SelectionRegistry&) override {
        log::info("VariablesPlugin::onInitialisation", "Defining variables...");
        if (!config_.contains("variables")) return;

        for (auto const& var_cfg : config_.at("variables")) {

            auto name   = var_cfg.at("name").get<std::string>();
            auto branch = var_cfg.at("branch").get<std::string>();
            auto label  = var_cfg.at("label").get<std::string>();
            auto strat  = var_cfg.at("stratum").get<std::string>();

            const auto& bins_cfg = var_cfg.at("bins");
            if (bins_cfg.is_string() && bins_cfg.get<std::string>() == "dynamic") {
                BinningDefinition placeholder_bins({-std::numeric_limits<double>::infinity(),
                                                  std::numeric_limits<double>::infinity()},
                                                 branch,
                                                 label,
                                                 {},
                                                 strat);
                def.addVariable(name, branch, label, placeholder_bins, strat, true, false);
            } else if (bins_cfg.is_object() && bins_cfg.contains("mode") && bins_cfg.at("mode") == "dynamic") {
                double domain_min = bins_cfg.value("min", -std::numeric_limits<double>::infinity());
                double domain_max = bins_cfg.value("max",  std::numeric_limits<double>::infinity());
                bool include_oob = bins_cfg.value("include_out_of_range_bins", true);
                BinningDefinition placeholder_bins({domain_min, domain_max},
                                                  branch,
                                                  label,
                                                  {},
                                                  strat);
                def.addVariable(name, branch, label, placeholder_bins, strat, true, include_oob);
            } else {
                std::vector<double> edges;
                if (bins_cfg.is_array()) {
                    edges = bins_cfg.get<std::vector<double>>();
                } else {
                    int n      = bins_cfg.at("n").get<int>();
                    double min = bins_cfg.at("min").get<double>();
                    double max = bins_cfg.at("max").get<double>();
                    for (int i = 0; i <= n; ++i)
                        edges.push_back(min + (max - min) * i / n);
                }
                BinningDefinition bins(edges, branch, label, {}, strat);
                def.addVariable(name, branch, label, bins, strat);
            }

            if (var_cfg.contains("regions")) {
                for (auto const& region_name : var_cfg.at("regions")) {
                    def.addVariableToRegion(region_name.get<std::string>(), name);
                }
            }
        }
    }

    void onPreSampleProcessing(const SampleKey&, const RegionKey&, const RunConfig&) override {}
    void onPostSampleProcessing(const SampleKey&, const RegionKey&, const RegionAnalysisMap&) override {}
    void onFinalisation(const RegionAnalysisMap&) override {}

private:
    nlohmann::json config_;
};

}

#endif
