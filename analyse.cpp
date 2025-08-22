#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <vector>
#include <ROOT/RDataFrame.hxx>

#include "AnalysisRunner.h"
#include "AnalysisDataLoader.h"
#include "SystematicsProcessor.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
#include "EventVariableRegistry.h"
#include "SelectionRegistry.h"
#include "StratifierRegistry.h"
#include "HistogramBooker.h"
#include "Logger.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        analysis::log::fatal("main", "Usage: ", argv[0], " <config.json> <plugins.json>");
        return 1;
    }

    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        analysis::log::fatal("main", "Could not open config file: ", argv[1]);
        return 1;
    }
    nlohmann::json config_data = nlohmann::json::parse(config_file);

    std::ifstream plugins_file(argv[2]);
    if (!plugins_file.is_open()) {
        analysis::log::fatal("main", "Could not open plugins file: ", argv[2]);
        return 1;
    }
    nlohmann::json plugins_config = nlohmann::json::parse(plugins_file);

    try {
        ROOT::EnableImplicitMT();

        std::string ntuple_base_directory = config_data.at("ntuple_base_directory").get<std::string>();

        for (auto const& [beam, run_configs] : config_data.at("run_configurations").items()) {
            analysis::log::info("main", "Starting analysis for beam: ", beam);

            std::vector<std::string> periods;
            for (auto const& [period, period_details] : run_configs.items()) {
                periods.push_back(period);
            }

            analysis::RunConfigRegistry rc_reg;
            analysis::RunConfigLoader::loadRunConfigurations(argv[1], rc_reg);

            analysis::EventVariableRegistry ev_reg;
            analysis::SelectionRegistry sel_reg;
            analysis::StratifierRegistry strat_reg;

            std::vector<analysis::KnobDef> knob_defs;
            for(const auto& [name, columns] : ev_reg.knobVariations()){
                knob_defs.push_back({name, columns.first, columns.second});
            }

            std::vector<analysis::UniverseDef> universe_defs;
            for(const auto& [name, n_universes] : ev_reg.multiUniverseVariations()){
                universe_defs.push_back({name, name, n_universes});
            }

            analysis::SystematicsProcessor sys_proc(knob_defs, universe_defs);

            analysis::AnalysisDataLoader data_loader(
                rc_reg,
                ev_reg,
                beam,
                periods,
                ntuple_base_directory,
                true
            );

            auto histogram_booker = std::make_unique<analysis::HistogramBooker>(strat_reg);

            analysis::AnalysisRunner runner(
                data_loader,
                sel_reg,
                ev_reg,
                std::move(histogram_booker),
                sys_proc,
                plugins_config
            );

            runner.run();
            analysis::log::info("main", "Finished analysis for beam:", beam);
        }

    } catch (const std::exception& e) {
        analysis::log::fatal("main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("main", "Analysis finished successfully.");
    return 0;
}