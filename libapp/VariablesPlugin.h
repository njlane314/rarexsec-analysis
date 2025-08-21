#ifndef VARIABLES_PLUGIN_H
#define VARIABLES_PLUGIN_H

#include "IAnalysisPlugin.h"
#include "AnalysisDefinition.h"
#include "Logger.h"
#include "BinDefinition.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

namespace analysis {

class VariablesPlugin : public IAnalysisPlugin {
public:
    explicit VariablesPlugin(const nlohmann::json& cfg)
        : config_(cfg) {}

    void onInitialisation(AnalysisDefinition& def,
                          const SelectionRegistry&) override {
        log::info("VariablesPlugin", "Defining variables...");
        if (!config_.contains("variables")) return;

        for (auto const& var_cfg : config_.at("variables")) {

            auto name   = var_cfg.at("name").get<std::string>();
            auto branch = var_cfg.at("branch").get<std::string>();
            auto label  = var_cfg.at("label").get<std::string>();
            auto strat  = var_cfg.at("stratum").get<std::string>();

            std::vector<double> edges;
            if (var_cfg.at("bins").is_array()) {
                edges = var_cfg.at("bins").get<std::vector<double>>();
            } else {
                int n      = var_cfg.at("bins").at("n").get<int>();
                double min = var_cfg.at("bins").at("min").get<double>();
                double max = var_cfg.at("bins").at("max").get<double>();
                for (int i = 0; i <= n; ++i)
                    edges.push_back(min + (max - min) * i / n);
            }
            
            BinDefinition bins(edges, branch, label, {}, name, strat);
            def.addVariable(name, branch, label, bins, strat);
        }
    }

private:
    nlohmann::json config_;
};

} 

#endif