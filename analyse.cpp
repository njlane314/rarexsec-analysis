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

int main(int argc, char *argv[]) {
    analysis::AnalysisLogger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    if (argc != 3) {
        analysis::log::fatal("analyse::main", "Invocation error. Expected:", argv[0], "<config.json> <plugins.json>");
        return 1;
    }

    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        analysis::log::fatal("analyse::main", "Configuration file inaccessible:", argv[1]);
        return 1;
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

        for (auto const &[beam, run_configs] : config_data.at("run_configurations").items()) {

            std::vector<std::string> periods;
            periods.reserve(run_configs.size());
            for (auto const &[period, period_details] : run_configs.items()) {
                periods.push_back(period);
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

            runner.run();
        }

    } catch (const std::exception &e) {
        analysis::log::fatal("analyse::main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("analyse::main", "Global analysis routine terminated nominally.");
    return 0;
}

