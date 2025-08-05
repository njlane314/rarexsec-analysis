// RUN_CONFIG_H
#ifndef RUN_CONFIG_H
#define RUN_CONFIG_H

#include <string>
#include <vector>
#include <set>
#include <nlohmann/json.hpp>
#include "SampleDefinition.h"
#include "Logger.h"

namespace analysis {

using json = nlohmann::json;

struct RunConfig {
    std::string                   beam_mode;
    std::string                   run_period;
    double                        nominal_pot{0.0};
    long                          nominal_triggers{0};
    std::vector<SampleDefinition> samples;

    RunConfig(const json& j, std::string bm, std::string pr)
      : beam_mode(std::move(bm))
      , run_period(std::move(pr))
      , nominal_pot(j.value("nominal_pot", 0.0))
      , nominal_triggers(j.value("nominal_triggers", 0L))
    {
        for (auto& sj : j.at("samples")) {
            // now matching your SampleDefinition ctor signature:
            samples.emplace_back(
              sj,
              /* base_dir */    j.value("base_dir", std::string{}),
              /* var_reg */     *reinterpret_cast<const EventVariableRegistry*>(nullptr),
              /* processor */   *reinterpret_cast<IEventProcessor*>(nullptr),
              /* blind */       false
            );
        }
    }

    std::string label() const {
        return beam_mode + ":" + run_period;
    }

    void validate(const std::string& baseDir) const {
        if (beam_mode.empty())     log::fatal("RunConfig", "empty beam_mode");
        if (run_period.empty())    log::fatal("RunConfig", "empty run_period");
        if (samples.empty())       log::fatal("RunConfig", "no samples for", beam_mode + "/" + run_period);

        std::set<std::string> keys;
        for (auto& s : samples) {
            s.validateFiles(baseDir);
            if (!keys.insert(s.internal_key_).second) {
                log::fatal("RunConfig", "duplicate sample key:", s.internal_key_);
            }
        }
    }
};

} // namespace analysis

#endif // RUN_CONFIG_H