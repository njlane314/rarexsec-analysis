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

int main(int argc, char *argv[]) {
    AnalysisLogger::getInstance().setLevel(LogLevel::DEBUG);

    if (argc != 3) {
        log::fatal("plot::main", "Invocation error.");
        return 1;
    }

    try {
        auto result = AnalysisResult::loadFromFile(argv[1]);

        if (!result) {
            log::fatal("plot::main", "Failed to load analysis result");
            return 1;
        }

        nlohmann::json cfg = loadJsonFile(argv[2]);
        auto analysis_cfg = cfg.at("analysis_configs");
        auto plot_cfg = cfg.at("plot_configs");

        RunConfigRegistry rc_reg;
        EventVariableRegistry ev_reg;
        std::map<std::string, std::unique_ptr<AnalysisDataLoader>> loaders;

        std::string ntuple_base_directory = analysis_cfg.at("ntuple_base_directory");

        RunConfigLoader::loadRunConfigurations(analysis_cfg, rc_reg);

        for (auto const &[beam, runs] : analysis_cfg.at("run_configurations").items()) {
            std::vector<std::string> periods;

            for (auto const &p : runs.items()) periods.push_back(p.key());

            loaders.emplace(beam, std::make_unique<AnalysisDataLoader>(rc_reg, ev_reg, beam, periods, ntuple_base_directory, true));
        }
        std::map<std::string, AnalysisResult> beam_results;

        for (auto const &kv : result->regions()) {
            beam_results[kv.second.beamConfig()].regions().insert(kv);
        }
        for (auto &kv : beam_results) {
            kv.second.build();
        }

        for (auto &kv : loaders) {
            AnalysisPluginManager manager;
            manager.loadPlugins(plot_cfg, kv.second.get());
            auto it = beam_results.find(kv.first);
            if (it != beam_results.end()) manager.notifyFinalisation(it->second);
        }
    } catch (const std::exception &e) {
        log::fatal("plot::main", "An error occurred:", e.what());
        return 1;
    }

    log::info("plot::main", "Plotting routine terminated nominally.");
    return 0;
}
