#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "AnalysisRunner.h"
#include "HistogramBooker.h"
#include "JsonUtils.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

static analysis::AnalysisResult runAnalysis(const nlohmann::json &samples, const nlohmann::json &analysis) {
    ROOT::EnableImplicitMT();
    analysis::log::info("analyse::runAnalysis", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(),
                        "threads.");

    std::string ntuple_directory = samples.at("ntuple_directory").get<std::string>();
    analysis::log::info("analyse::runAnalysis", "Configuration loaded for", samples.at("beamlines").size(),
                        "beamlines.");

    analysis::RunConfigRegistry run_config_registry;
    analysis::RunConfigLoader::loadRunConfigurations(samples, run_config_registry);

    analysis::AnalysisResult result;
    for (auto const &[beam, runs] : samples.at("beamlines").items()) {
        std::vector<std::string> periods;
        periods.reserve(runs.size());
        for (auto const &[period, _] : runs.items()) {
            periods.emplace_back(period);
        }

        analysis::VariableRegistry variable_registry;

        std::vector<analysis::KnobDef> knob_defs;
        knob_defs.reserve(variable_registry.knobVariations().size());
        for (const auto &kv : variable_registry.knobVariations()) {
            knob_defs.push_back({kv.first, kv.second.first, kv.second.second});
        }

        std::vector<analysis::UniverseDef> universe_defs;
        universe_defs.reserve(variable_registry.multiUniverseVariations().size());
        for (const auto &kv : variable_registry.multiUniverseVariations()) {
            universe_defs.push_back({kv.first, kv.first, kv.second});
        }

        analysis::SystematicsProcessor systematics_processor(knob_defs, universe_defs);
        analysis::AnalysisDataLoader data_loader(run_config_registry, variable_registry, beam, periods,
                                                 ntuple_directory, true);
        auto histogram_booker = std::make_unique<analysis::HistogramBooker>();

        analysis::AnalysisRunner runner(data_loader, variable_registry, std::move(histogram_booker),
                                        systematics_processor, analysis);
        auto beamline_result = runner.run();
        for (auto &kv : beamline_result.regions()) {
            result.regions().insert(kv);
        }
    }

    return result;
}

int main(int argc, char *argv[]) {
    analysis::AnalysisLogger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc != 4) {
        analysis::log::fatal("analyse::main", "Invocation error. Expected:", argv[0],
                             "<samples.json> <plugins.json> <output.root>");
        return 1;
    }

    try {
        nlohmann::json cfg = analysis::loadJsonFile(argv[1]);
        nlohmann::json plg = analysis::loadJsonFile(argv[2]);
        const char *out_path = argv[3];

        auto result = runAnalysis(cfg.at("samples"), plg.at("analysis"));
        result.saveToFile(out_path);
    } catch (const std::exception &e) {
        analysis::log::fatal("analyse::main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("analyse::main", "Global analysis routine terminated nominally.");
    return 0;
}
