#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "AnalysisRunner.h"
#include "EventVariableRegistry.h"
#include "HistogramBooker.h"
#include "JsonUtils.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"
#include "SystematicsProcessor.h"

static analysis::AnalysisResult runAnalysis(const nlohmann::json &config_data, const nlohmann::json &config_analysis) {
    ROOT::EnableImplicitMT();
    analysis::log::info("analyse::main", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(), "threads.");

    std::string ntuple_base_directory = config_data.at("ntuple_base_directory").get<std::string>();
    analysis::log::info("analyse::main", "Configuration loaded for", config_data.at("run_configurations").size(), "beamlines.");

    analysis::RunConfigRegistry rc_reg;
    analysis::RunConfigLoader::loadRunConfigurations(config_data, rc_reg);

    analysis::AnalysisResult combined_res;
    for (auto const &[beam, config_run] : config_data.at("run_configurations").items()) {
        std::vector<std::string> periods;
        periods.reserve(config_run.size());
        for (const auto &item : config_run.items()) {
            periods.push_back(item.key());
        }
        analysis::EventVariableRegistry ev_reg;
        analysis::SelectionRegistry sel_reg;
        analysis::StratifierRegistry strat_reg;
        std::vector<analysis::KnobDef> knob_defs;
        knob_defs.reserve(ev_reg.knobVariations().size());
        for (const auto &kv : ev_reg.knobVariations()) {
            knob_defs.push_back({kv.first, kv.second.first, kv.second.second});
        }
        std::vector<analysis::UniverseDef> universe_defs;
        universe_defs.reserve(ev_reg.multiUniverseVariations().size());
        for (const auto &kv : ev_reg.multiUniverseVariations()) {
            universe_defs.push_back({kv.first, kv.first, kv.second});
        }
        analysis::SystematicsProcessor sys_proc(knob_defs, universe_defs);
        analysis::AnalysisDataLoader data_loader(rc_reg, ev_reg, beam, periods, ntuple_base_directory, true);
        auto histogram_booker = std::make_unique<analysis::HistogramBooker>(strat_reg);
        analysis::AnalysisRunner runner(data_loader, sel_reg, ev_reg, std::move(histogram_booker), sys_proc, config_analysis);
        auto res = runner.run();
        for (auto &kv : res.regions()) {
            combined_res.regions().insert(kv);
        }
    }

    return combined_res;
}

int main(int argc, char *argv[]) {
    analysis::AnalysisLogger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc != 4) {
        analysis::log::fatal("analyse::main", "Invocation error. Expected:", argv[0], "<config.json> <plugins.json> <output.root>");
        return 1;
    }

    try {
        nlohmann::json cfg = analysis::loadJsonFile(argv[1]);
        nlohmann::json plg;
        const char *out_path;
        plg = analysis::loadJsonFile(argv[2]);
        out_path = argv[3];
        
        auto result = runAnalysis(cfg.at("analysis_configs"), plg);
        result.saveToFile(out_path);
    } catch (const std::exception &e) {
        analysis::log::fatal("analyse::main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("analyse::main", "Global analysis routine terminated nominally.");
    return 0;
}

