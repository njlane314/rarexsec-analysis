#include <map>
#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "Logger.h"
#include "AnalysisResult.h"
#include "JsonUtils.h"
#include "PlotPluginManager.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "VariableRegistry.h"

static int runPlotting(const nlohmann::json &samples, const nlohmann::json &plotting,
                       const analysis::AnalysisResult &result) {
    ROOT::EnableImplicitMT();
    analysis::log::info("plot::runPlotting", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(), "threads.");

    std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
    analysis::log::info("plot::runPlotting", "Configuration loaded for", samples.at("beamlines").size(), "beamlines.");

    analysis::RunConfigRegistry run_config_registry;
    analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

    analysis::VariableRegistry variable_registry;
    std::map<std::string, std::unique_ptr<analysis::AnalysisDataLoader>> loaders;

    for (auto const &[beam, runs] : samples.at("beamlines").items()) {
        std::vector<std::string> periods;
        periods.reserve(runs.size());
        for (auto const &[period, _] : runs.items()) {
            periods.emplace_back(period);
        }

        loaders.emplace(beam, std::make_unique<analysis::AnalysisDataLoader>(run_config_registry, variable_registry, beam, periods, ntuple_dir, true));
    }

    auto result_map = result.resultsByBeam();
    for (auto &[beam, loader] : loaders) {
        analysis::PlotPluginManager manager;
        manager.loadPlugins(plotting, loader.get());
          auto it = result_map.find(beam);
          if (it != result_map.end())
              manager.notifyPlot(it->second);
    }

    analysis::log::info("plot::main", "Plotting routine terminated nominally.");
    return 0;
}

int main(int argc, char *argv[]) {
    analysis::Logger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc != 4) {
        analysis::log::fatal("plot::main", "Invocation error. Expected:", argv[0], "<samples.json> <plugins.json> <input.root>");
        return 1;
    }

    try {
        nlohmann::json cfg = analysis::loadJson(argv[1]);
        nlohmann::json plg = analysis::loadJson(argv[2]);

        auto result = analysis::AnalysisResult::loadFromFile(argv[3]);
        if (!result) {
            analysis::log::fatal("plot::main", "Failed to load", argv[3], "analysis result.");
            return 1;
        }

        return runPlotting(cfg.at("samples"), plg.at("plotting"), *result);
    } catch (const std::exception &e) {
        analysis::log::fatal("plot::main", "An error occurred:", e.what());
        return 1;
    }
}
