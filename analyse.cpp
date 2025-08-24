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
    analysis::Logger::getInstance().setLevel(analysis::LogLevel::DEBUG);

    std::cout << "\033[38;5;51m" << R"(
 ____      /\\  ____   _____ __  __  ____   _____   ____  
|  _ \\    /  \\|  _ \\ | ____|\\ \\/ / / ___| | ____| / ___| 
| |_) |  / /\\ \\| |_) ||  _|   >  <  \\___ \\ |  _|  | |     
|  _ <  / ____ \\|  _ < | |___ /_/\\_\\ ___) || |___ | |___  
|_| \\_\\/_/    \\_\\|_| \\_\\|_____|       |____/ |_____| \\____|
)" << "\033[0m" << std::endl;
    std::cout << "\033[1;90mν RareXSec Cross-Section Analysis Pipeline\n"
                 "---------------------------------------------------------------\033[0m" << std::endl;

    if (argc != 3) {
        analysis::log::fatal("main", "Invocation error. Expected:", argv[0], "<config.json> <plugins.json>");
        return 1;
    }

    std::ifstream config_file(argv[1]);
    if (!config_file.is_open()) {
        analysis::log::fatal("main", "Configuration file inaccessible:", argv[1]);
        return 1;
    }
    nlohmann::json config_data = nlohmann::json::parse(config_file);

    std::ifstream plugins_file(argv[2]);
    if (!plugins_file.is_open()) {
        analysis::log::fatal("main", "Plugin manifest inaccessible:", argv[2]);
        return 1;
    }
    nlohmann::json plugins_config = nlohmann::json::parse(plugins_file);

    try {
        ROOT::EnableImplicitMT();
        analysis::log::info("main", "Implicit multithreading engaged across", ROOT::GetThreadPoolSize(), "threads.");

        std::string ntuple_base_directory = config_data.at("ntuple_base_directory").get<std::string>();

        analysis::log::info("main", "Configuration loaded for", config_data.at("run_configurations").size(), "beamlines.");

        for (auto const& [beam, run_configs] : config_data.at("run_configurations").items()) {
            analysis::log::info("main", "Initiating analysis sequence for beam:", beam);

            std::vector<std::string> periods;
            for (auto const& [period, period_details] : run_configs.items()) {
                periods.push_back(period);
            }

            analysis::RunConfigRegistry rc_reg;
            analysis::RunConfigLoader::loadRunConfigurations(argv[1], rc_reg);
            analysis::log::info("main", "Configuration registry initialised with", rc_reg.all().size(), "entries.");

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

            analysis::log::info("main", "Commencing data ingestion phase for", periods.size(), "periods...");
            analysis::AnalysisDataLoader data_loader(
                rc_reg,
                ev_reg,
                beam,
                periods,
                ntuple_base_directory,
                true
            );
            analysis::log::info("main", "Data ingestion phase concluded with", data_loader.getSampleFrames().size(), "samples prepared.");

            auto histogram_booker = std::make_unique<analysis::HistogramBooker>(strat_reg);

            analysis::log::info("main", "Calibrating analysis subsystems...");
            analysis::AnalysisRunner runner(
                data_loader,
                sel_reg,
                ev_reg,
                std::move(histogram_booker),
                sys_proc,
                plugins_config
            );

            analysis::log::info("main", "Dispatching analysis execution...");
            runner.run();
            analysis::log::info("main", "Beam processing cycle complete:", beam);
        }

    } catch (const std::exception& e) {
        analysis::log::fatal("main", "An error occurred:", e.what());
        return 1;
    }

    analysis::log::info("main", "Global analysis routine terminated nominally.");
    return 0;
}
