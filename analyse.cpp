#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
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
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"
#include "SystematicsProcessor.h"

namespace fs = std::filesystem;

static nlohmann::json loadJsonFile(const std::string &path) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        analysis::log::fatal("analyse::loadJsonFile", "File inaccessible:", path);
        throw std::runtime_error("File inaccessible");
    }
    std::ifstream file(path);
    if (!file.is_open()) {
        analysis::log::fatal("analyse::loadJsonFile", "Unable to open file:", path);
        throw std::runtime_error("Unable to open file");
    }

    nlohmann::json config_data = nlohmann::json::parse(config_file);

    std::ifstream plugins_file(argv[2]);
    if (!plugins_file.is_open()) {
        analysis::log::fatal("analyse::main", "Plugin manifest inaccessible:", argv[2]);
        return 1;
    }
    nlohmann::json plugins_config = nlohmann::json::parse(plugins_file);

    try {
        ROOT::EnableImplicitMT();
        analysis::log::info("analyse::main", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(),
                            "threads.");

        std::string ntuple_base_directory = config_data.at("ntuple_base_directory").get<std::string>();

        analysis::log::info("analyse::main", "Configuration loaded for", config_data.at("run_configurations").size(),
                            "beamlines.");

        analysis::RunConfigRegistry run_config_registry;
        analysis::RunConfigLoader::loadRunConfigurations(argv[1], run_config_registry);

    return nlohmann::json::parse(file);
}

static analysis::SystematicsProcessor makeSystematicsProcessor(
    const analysis::EventVariableRegistry &ev_reg) {
    std::vector<analysis::KnobDef> knob_defs;
    knob_defs.reserve(ev_reg.knobVariations().size());
    std::transform(ev_reg.knobVariations().begin(), ev_reg.knobVariations().end(),
                   std::back_inserter(knob_defs),
                   [](const auto &kv) {
                       return analysis::KnobDef{kv.first, kv.second.first, kv.second.second};
                   });

    std::vector<analysis::UniverseDef> universe_defs;
    universe_defs.reserve(ev_reg.multiUniverseVariations().size());
    std::transform(ev_reg.multiUniverseVariations().begin(),
                   ev_reg.multiUniverseVariations().end(),
                   std::back_inserter(universe_defs),
                   [](const auto &kv) {
                       return analysis::UniverseDef{kv.first, kv.first, kv.second};
                   });

    return analysis::SystematicsProcessor(knob_defs, universe_defs);
}

struct AnalysisComponents {
    analysis::EventVariableRegistry ev_reg;
    analysis::SelectionRegistry sel_reg;
    analysis::StratifierRegistry strat_reg;
    analysis::SystematicsProcessor sys_proc;

    AnalysisComponents() : sys_proc(makeSystematicsProcessor(ev_reg)) {}
};

static void runBeamline(const std::string &beam, const nlohmann::json &run_configs,
                        const std::string &ntuple_base_directory,
                        analysis::RunConfigRegistry &rc_reg,
                        const nlohmann::json &plugins_config) {
    std::vector<std::string> periods;
    periods.reserve(run_configs.size());
    std::transform(run_configs.items().begin(), run_configs.items().end(),
                   std::back_inserter(periods),
                   [](const auto &item) { return item.key(); });

    AnalysisComponents components;

    analysis::AnalysisDataLoader data_loader(rc_reg, components.ev_reg, beam, periods,
                                             ntuple_base_directory, true);

    auto histogram_booker = std::make_unique<analysis::HistogramBooker>(components.strat_reg);

    analysis::AnalysisRunner runner(data_loader, components.sel_reg, components.ev_reg,
                                    std::move(histogram_booker), components.sys_proc,
                                    plugins_config);

    runner.run();
}

            analysis::EventVariableRegistry event_variable_registry;
            analysis::SelectionRegistry selection_registry;
            analysis::StratifierRegistry stratifier_registry;

            std::vector<analysis::KnobDef> knob_defs;
            knob_defs.reserve(event_variable_registry.knobVariations().size());
            for (const auto &[name, columns] : event_variable_registry.knobVariations()) {
                knob_defs.push_back({name, columns.first, columns.second});
            }

            std::vector<analysis::UniverseDef> universe_defs;
            universe_defs.reserve(event_variable_registry.multiUniverseVariations().size());
            for (const auto &[name, n_universes] : event_variable_registry.multiUniverseVariations()) {
                universe_defs.push_back({name, name, n_universes});
            }

            analysis::SystematicsProcessor systematics_processor(knob_defs, universe_defs);

            // Instantiate the data loader outside of the plugin infrastructure
            // so that it remains alive for the entire duration of the analysis
            // run.  Plugins such as EventDisplay rely on this object during
            // their finalisation stage, so destroying it earlier would leave
            // them without access to the required event data.
            analysis::AnalysisDataLoader data_loader(run_config_registry, event_variable_registry, beam, periods,
                                                    ntuple_base_directory, true);

            auto histogram_booker = std::make_unique<analysis::HistogramBooker>(stratifier_registry);

            analysis::AnalysisRunner runner(data_loader, selection_registry, event_variable_registry,
                                            std::move(histogram_booker), systematics_processor, plugins_config);


    try {
        nlohmann::json config_data = loadJsonFile(argv[1]);
        nlohmann::json plugins_config = loadJsonFile(argv[2]);

        runAnalysis(config_data, plugins_config, argv[1]);
    } catch (const std::exception &e) {
        analysis::log::fatal("analyse::main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("analyse::main",
                         "Global analysis routine terminated nominally.");
    return 0;
}

