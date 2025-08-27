#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include <fstream>
#include <string>

#include "nlohmann/json.hpp"

#include "AnalysisLogger.h"
#include "RunConfig.h"
#include "RunConfigRegistry.h"

namespace analysis {

class RunConfigLoader {
  public:
    static inline void loadRunConfigurations(const nlohmann::json &data,
                                             RunConfigRegistry &registry) {
        for (auto const &[beam, run_configs] : data.at("run_configurations").items()) {
            for (auto const &[run_period, run_details] : run_configs.items()) {
                RunConfig config(run_details, beam, run_period);
                config.validate();
                registry.addConfig(std::move(config));
            }
        }
    }
    static inline void loadRunConfigurations(const std::string &config_path,
                                             RunConfigRegistry &registry) {
        std::ifstream f(config_path);
        if (!f.is_open()) {
            log::fatal("RunConfigLoader::loadRunConfigurations",
                       "Could not open config file:", config_path);
        }
        nlohmann::json data = nlohmann::json::parse(f);
        loadRunConfigurations(data, registry);
    }
};

}

#endif