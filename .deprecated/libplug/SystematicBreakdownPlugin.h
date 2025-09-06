#ifndef SYSTEMATICBREAKDOWNPLUGIN_H
#define SYSTEMATICBREAKDOWNPLUGIN_H

#include <stdexcept>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include <TSystem.h>

#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "SystematicBreakdownPlot.h"

namespace analysis {

class SystematicBreakdownPlugin : public IAnalysisPlugin {
  public:
    struct PlotConfig {
        std::string variable;
        std::string region;
        std::string output_directory = "plots";
        bool fractional = false;
    };

    explicit SystematicBreakdownPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("plots") || !cfg.at("plots").is_array())
            throw std::runtime_error("SystematicBreakdownPlugin missing plots");
        for (auto const &p : cfg.at("plots")) {
            PlotConfig pc;
            pc.variable = p.at("variable").get<std::string>();
            pc.region = p.at("region").get<std::string>();
            pc.output_directory = p.value("output_directory", std::string("plots"));
            pc.fractional = p.value("fractional", false);
            plots_.push_back(std::move(pc));
        }
    }

    void onInitialisation(AnalysisDefinition &, const SelectionRegistry &) override {}
    void onPreSampleProcessing(const SampleKey &, const RegionKey &, const RunConfig &) override {}
    void onPostSampleProcessing(const SampleKey &, const RegionKey &, const RegionAnalysisMap &) override {}

    void onFinalisation(const RegionAnalysisMap &region_map) override {
        gSystem->mkdir("plots", true);
        for (auto const &pc : plots_) {
            RegionKey rkey{pc.region};
            auto it = region_map.find(rkey);

            if (it == region_map.end()) {
                log::error("SystematicBreakdownPlugin::onFinalisation",
                           "Could not find analysis region for key:", rkey.str());
                continue;
            }

            VariableKey vkey{pc.variable};
            if (!it->second.hasFinalVariable(vkey)) {
                log::error("SystematicBreakdownPlugin::onFinalisation", "Could not find variable", vkey.str(),
                           "in region", rkey.str());
                continue;
            }

            const auto &region_analysis = it->second;
            const auto &variable_result = region_analysis.getFinalVariable(vkey);

            SystematicBreakdownPlot plot("syst_breakdown_" + pc.variable + "_" + pc.region, variable_result,
                                         pc.fractional, pc.output_directory);
            plot.drawAndSave("pdf");
        }
    }

  private:
    std::vector<PlotConfig> plots_;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::SystematicBreakdownPlugin(cfg);
}
#endif

#endif
