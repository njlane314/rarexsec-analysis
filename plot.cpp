#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "AnalysisPluginManager.h"
#include "AnalysisResult.h"
#include "EventVariableRegistry.h"
#include "RunConfigLoader.h"
#include "RunConfigRegistry.h"
using namespace analysis;
namespace fs = std::filesystem;
static nlohmann::json loadJsonFile(const std::string &path) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) throw std::runtime_error("File inaccessible");
    std::ifstream file(path);
    if (!file.is_open()) throw std::runtime_error("Unable to open file");
    return nlohmann::json::parse(file);
}
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
        nlohmann::json plugins_config = loadJsonFile(argv[2]);
        RunConfigRegistry rc_reg;
        EventVariableRegistry ev_reg;
        std::map<std::string, std::unique_ptr<AnalysisDataLoader>> loaders;
        if (plugins_config.contains("analysis_config")) {
            std::string cfg_path = plugins_config.at("analysis_config");
            nlohmann::json config_data = loadJsonFile(cfg_path);
            RunConfigLoader::loadRunConfigurations(cfg_path, rc_reg);
            std::string ntuple_base_directory = config_data.at("ntuple_base_directory");
            for (auto const &[beam, runs] : config_data.at("run_configurations").items()) {
                std::vector<std::string> periods;
                for (auto const &p : runs.items()) periods.push_back(p.key());
                loaders.emplace(beam, std::make_unique<AnalysisDataLoader>(rc_reg, ev_reg, beam, periods, ntuple_base_directory, true));
            }
        }
        std::map<std::string, AnalysisPluginManager::RegionAnalysisMap> beam_regions;
        for (auto const &kv : result->regions()) {
            beam_regions[kv.second.beamConfig()].insert(kv);
        }
        if (loaders.empty()) {
            AnalysisPluginManager manager;
            manager.loadPlugins(plugins_config, nullptr);
            manager.notifyFinalisation(result->regions());
        } else {
            for (auto &kv : loaders) {
                AnalysisPluginManager manager;
                manager.loadPlugins(plugins_config, kv.second.get());
                auto it = beam_regions.find(kv.first);
                if (it != beam_regions.end()) manager.notifyFinalisation(it->second);
            }
        }
    } catch (const std::exception &e) {
        log::fatal("plot::main", "An error occurred:", e.what());
        return 1;
    }
    log::info("plot::main", "Plotting routine terminated nominally.");
    return 0;
}
