#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "AnalysisRunner.h"
#include "HistogramFactory.h"
#include "JsonUtils.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

static analysis::AnalysisResult processBeamline(analysis::RunConfigRegistry &run_config_registry,
                                                const std::string &ntuple_dir, const std::string &beam,
                                                const nlohmann::json &runs, const nlohmann::json &analysis) {
    std::vector<std::string> periods;
    periods.reserve(runs.size());

    for (auto const &[period, _] : runs.items()) {
        periods.emplace_back(period);
    }

    analysis::VariableRegistry variable_registry;
    analysis::SystematicsProcessor systematics_processor(variable_registry);
    analysis::AnalysisDataLoader data_loader(run_config_registry, variable_registry, beam, periods, ntuple_dir, true);
    auto histogram_factory = std::make_unique<analysis::HistogramFactory>();

    analysis::AnalysisRunner runner(data_loader, variable_registry, std::move(histogram_factory), systematics_processor, analysis);

    return runner.run();
}

static void aggregateResults(analysis::AnalysisResult &result, const analysis::AnalysisResult &beamline_result) {
    for (auto &kv : beamline_result.regions()) {
        result.regions().insert(kv);
    }
}

static analysis::AnalysisResult runAnalysis(const nlohmann::json &samples, const nlohmann::json &analysis) {
    ROOT::EnableImplicitMT();
    analysis::log::info("analyse::runAnalysis", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(), "threads.");

    std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
    analysis::log::info("analyse::runAnalysis", "Configuration loaded for", samples.at("beamlines").size(), "beamlines.");

    analysis::RunConfigRegistry run_config_registry;
    analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

    analysis::AnalysisResult result;
    for (auto const &[beam, runs] : samples.at("beamlines").items()) {
        auto beamline_result = processBeamline(run_config_registry, ntuple_dir, beam, runs, analysis);
        aggregateResults(result, beamline_result);
    }

    return result;
}

int main(int argc, char *argv[]) {
    analysis::AnalysisLogger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc != 4) {
        analysis::log::fatal("analyse::main", "Invocation error. Expected:", argv[0], "<samples.json> <plugins.json> <output.root>");
        return 1;
    }

    try {
        nlohmann::json cfg = analysis::loadJson(argv[1]);
        nlohmann::json plg = analysis::loadJson(argv[2]);
        const char *output_path = argv[3];

        auto result = runAnalysis(cfg.at("samples"), plg.at("analysis"));
        result.saveToFile(output_path);
    } catch (const std::exception &e) {
        analysis::log::fatal("analyse::main", "An error occurred:", e.what());
        return 1;
    }
    
    analysis::log::info("analyse::main", "Global analysis routine terminated nominally.");
    
    return 0;
}
