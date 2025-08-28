#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "AnalysisPluginManager.h"
#include "AnalysisResult.h"
#include "EventVariableRegistry.h"
#include "JsonUtils.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"

using namespace analysis;

static void runPlotting(const nlohmann::json &config_data, const nlohmann::json &config_plot, const AnalysisResult &res) {
    ROOT::EnableImplicitMT();
    analysis::log::info("plot::runPlotting", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(), "threads.");

    std::string ntuple_base_directory = config_data.at("ntuple_base_directory");
    analysis::log::info("plot::runPlotting", "Configuration loaded for", config_data.at("run_configurations").size(), "beamlines.");

    RunConfigRegistry rc_reg;
    RunConfigLoader::loadRunConfigurations(config_data, rc_reg);

    EventVariableRegistry ev_reg;
    std::map<std::string, std::unique_ptr<AnalysisDataLoader>> loaders;
    
    for (auto const &[beam, config_run] : config_data.at("run_configurations").items()) {
        std::vector<std::string> periods;
        for (auto const &p : config_run.items()) {
            periods.push_back(p.key());
        }

        loaders.emplace(beam, std::make_unique<AnalysisDataLoader>(rc_reg, ev_reg, beam, periods, ntuple_base_directory, true));
    }

    auto beam_results = [&res] {
        std::map<std::string, AnalysisResult> m;
        for (auto const &kv : res.regions()) m[kv.second.beamConfig()].regions().insert(kv);

        for (auto &[k, v] : m) 
            v.build();
        
        return m;
    }

    for (auto &[beam, loader] : loaders) {
        AnalysisPluginManager manager;
        manager.loadPlugins(config_plot, loader.get());
        auto it = beam_results.find(beam);
        if (it != beam_results.end()) manager.notifyFinalisation(it->second); 
    }
}

int main(int argc, char *argv[]) {
    AnalysisLogger::getInstance().setLevel(LogLevel::DEBUG);

    if (argc != 4) {
        log::fatal("plot::main", "Invocation error. Expected:", argv[0], "<config.json> <plugins.json> <input.root>");
        return 1;
    }

    try {
        nlohmann::json cfg = loadJsonFile(argv[1]);
        nlohmann::json plg = loadJsonFile(argv[2]);

        auto result = AnalysisResult::loadFromFile(argv[3]);
        if (!result) {
            log::fatal("plot::main", "Failed to load", argv[3], "analysis result.");
            return 1;
        }

        runPlotting(cfg.at("analysis_configs"), plg, *result);
    } catch (const std::exception &e) {
        log::fatal("plot::main", "An error occurred:", e.what());
        return 1;
    }

    log::info("plot::main", "Plotting routine terminated nominally.");
    return 0;
}
