#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <ROOT/RDataFrame.hxx>
#include <nlohmann/json.hpp>

#include "AnalysisDataLoader.h"
#include "AnalysisResult.h"
#include "AnalysisRunner.h"
#include "HistogramFactory.h"
#include "Json.h"
#include "Logger.h"
#include "PipelineBuilder.h"
#include "PluginAliases.h"
#include "PluginSpec.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SystematicsProcessor.h"
#include "VariableRegistry.h"

namespace {

using analysis::PluginArgs;
using analysis::PluginSpecList;
using analysis::Target;

// Convert a list of plugin specifications to the JSON format
// consumed by AnalysisRunner's constructor.
static nlohmann::json specsToJson(const PluginSpecList &specs) {
    nlohmann::json arr = nlohmann::json::array();
    for (auto const &s : specs) {
        arr.push_back({{"id", s.id}, {"args", s.args}});
    }
    return nlohmann::json{{"plugins", arr}};
}

// Build separate analysis and plot plugin specification lists using
// the PipelineBuilder. This supports presets and explicit plugins in
// the configuration.
static std::pair<PluginSpecList, PluginSpecList>
buildPipeline(const nlohmann::json &cfg) {
    analysis::AnalysisPluginHost a_host; // dummy hosts
    analysis::PlotPluginHost p_host;
    analysis::PipelineBuilder builder(a_host, p_host);

    if (cfg.contains("presets")) {
        for (auto const &p : cfg.at("presets")) {
            std::string name = p.at("name").get<std::string>();
            PluginArgs vars = p.value("vars", PluginArgs::object());
            std::unordered_map<std::string, PluginArgs> overrides;
            if (p.contains("overrides")) {
                for (auto const &[k, v] : p.at("overrides").items()) {
                    overrides.emplace(k, v);
                }
            }
            builder.use(name, vars, overrides);
        }
    }

    if (cfg.contains("plugins")) {
        for (auto const &p : cfg.at("plugins")) {
            std::string tgt = p.value("target", std::string("analysis"));
            Target target = Target::Analysis;
            if (tgt == "plot")
                target = Target::Plot;
            else if (tgt == "both")
                target = Target::Both;

            std::string id = p.at("id").get<std::string>();
            PluginArgs args = p.value("args", PluginArgs::object());
            builder.add(target, id, args);
        }
    }

    builder.uniqueById();
    PluginSpecList a_specs = builder.analysisSpecs();
    PluginSpecList p_specs = builder.plotSpecs();
    return {a_specs, p_specs};
}

static analysis::AnalysisResult
processBeamline(analysis::RunConfigRegistry &run_config_registry,
                const std::string &ntuple_dir, const std::string &beam,
                const nlohmann::json &runs,
                const PluginSpecList &analysis_specs) {
    std::vector<std::string> periods;
    periods.reserve(runs.size());
    for (auto const &[period, _] : runs.items())
        periods.emplace_back(period);

    analysis::VariableRegistry variable_registry;
    analysis::SystematicsProcessor systematics_processor(variable_registry);
    analysis::AnalysisDataLoader data_loader(run_config_registry,
                                             variable_registry, beam, periods,
                                             ntuple_dir, true);
    auto histogram_factory =
        std::make_unique<analysis::HistogramFactory>();

    nlohmann::json plugin_cfg = specsToJson(analysis_specs);
    analysis::AnalysisRunner runner(data_loader, std::move(histogram_factory),
                                    systematics_processor, plugin_cfg);

    return runner.run();
}

static void aggregateResults(analysis::AnalysisResult &result,
                             const analysis::AnalysisResult &beamline_result) {
    for (auto &kv : beamline_result.regions())
        result.regions().insert(kv);
}

static analysis::AnalysisResult
runAnalysis(const nlohmann::json &samples,
            const PluginSpecList &analysis_specs) {
    ROOT::EnableImplicitMT();
    analysis::log::info("analysis::runAnalysis",
                        "Implicit multithreading engaged across",
                        ROOT::GetThreadPoolSize(), "threads.");

    std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
    analysis::log::info("analysis::runAnalysis", "Configuration loaded for",
                        samples.at("beamlines").size(), "beamlines.");

    analysis::RunConfigRegistry run_config_registry;
    analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

    analysis::AnalysisResult result;
    for (auto const &[beam, runs] : samples.at("beamlines").items()) {
        auto beamline_result = processBeamline(run_config_registry, ntuple_dir,
                                               beam, runs, analysis_specs);
        aggregateResults(result, beamline_result);
    }

    return result;
}

static void plotBeamline(analysis::RunConfigRegistry &run_config_registry,
                         const std::string &ntuple_dir,
                         const std::string &beam,
                         const nlohmann::json &runs,
                         const PluginSpecList &plot_specs,
                         const analysis::AnalysisResult &beam_result) {
    std::vector<std::string> periods;
    periods.reserve(runs.size());
    for (auto const &[period, _] : runs.items())
        periods.emplace_back(period);

    analysis::VariableRegistry variable_registry;
    analysis::AnalysisDataLoader data_loader(run_config_registry,
                                             variable_registry, beam, periods,
                                             ntuple_dir, true);

    analysis::PlotPluginHost p_host(&data_loader);
    for (auto const &spec : plot_specs)
        p_host.add(spec.id, spec.args);

    p_host.forEach([&](analysis::IPlotPlugin &pl) { pl.onPlot(beam_result); });
}

static void runPlotting(const nlohmann::json &samples,
                        const PluginSpecList &plot_specs,
                        const analysis::AnalysisResult &result) {
    std::string ntuple_dir = samples.at("ntupledir").get<std::string>();
    analysis::log::info("analysis::runPlotting", "Configuration loaded for",
                        samples.at("beamlines").size(), "beamlines.");

    analysis::RunConfigRegistry run_config_registry;
    analysis::RunConfigLoader::loadFromJson(samples, run_config_registry);

    auto result_map = result.resultsByBeam();
    for (auto const &[beam, runs] : samples.at("beamlines").items()) {
        auto it = result_map.find(beam);
        if (it != result_map.end()) {
            plotBeamline(run_config_registry, ntuple_dir, beam, runs,
                         plot_specs, it->second);
        }
    }

    analysis::log::info("analysis::runPlotting",
                        "Plotting routine terminated nominally.");
}

} // namespace

int main(int argc, char *argv[]) {
    analysis::Logger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc < 3 || argc > 4) {
        analysis::log::fatal("analysis::main",
                             "Invocation error. Expected:", argv[0],
                             "<samples.json> <pipeline.json> [output.root]");
        return 1;
    }

    try {
        nlohmann::json cfg = analysis::loadJson(argv[1]);
        nlohmann::json plg = analysis::loadJson(argv[2]);

        auto [analysis_specs, plot_specs] =
            buildPipeline(plg.at("pipeline"));

        const char *user_env = std::getenv("USER");
        std::string scratch_dir = "/pnfs/uboone/scratch/users/";
        scratch_dir += user_env ? user_env : "nlane";
        scratch_dir += "/results/";

        std::string output_name;
        if (argc == 4) {
            output_name = argv[3];
        } else {
            std::filesystem::path samples_path(argv[1]);
            std::string dataset = samples_path.stem().string();
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&t);
            std::ostringstream oss;
            oss << std::put_time(&tm, "%Y%m%d");
            output_name = "analysis_" + dataset + "_" + oss.str() + ".root";
        }

        std::string output_path = scratch_dir + output_name;
        analysis::log::info("analysis::main", "Writing analysis output to",
                            output_path);

        auto result = runAnalysis(cfg.at("samples"), analysis_specs);
        result.saveToFile(output_path.c_str());

        runPlotting(cfg.at("samples"), plot_specs, result);
    } catch (const std::exception &e) {
        analysis::log::fatal("analysis::main", "An error occurred:",
                             e.what());
        return 1;
    }

    analysis::log::info("analysis::main",
                        "Global analysis routine terminated nominally.");
    return 0;
}

