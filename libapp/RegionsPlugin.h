#include "IAnalysisPlugin.h"
#include "AnalysisDefinition.h"
#include "Logger.h"
#include <nlohmann/json.hpp>

using namespace analysis;

class RegionsPlugin : public IAnalysisPlugin 
{
    nlohmann::json config_;

public:
    explicit RegionsPlugin(const nlohmann::json& cfg) : config_(cfg) {}

    void onInitialisation(const AnalysisDefinition& def,
                          const SelectionRegistry&) override {
        log::info("RegionsPlugin", "Defining regions...");
        if (!config_.contains("regions")) return;

        for (auto const& region_cfg : config_.at("regions")) {
            auto id    = region_cfg.at("id").get<std::string>();
            auto label = region_cfg.at("label").get<std::string>();
            auto cuts  = region_cfg.at("cuts").get<std::vector<std::string>>();
            def.addRegion(id, label, {cuts.begin(), cuts.end()});
        }
    }

    void onPreSampleProcessing(const std::string&, const RegionConfig&, const std::string&) override {}

    void onPostSampleProcessing(const std::string&, const std::string&, const HistogramResult&) override {}

    void onFinalisation(const HistogramResult&) override {}
};

extern "C" analysis::IAnalysisPlugin* createPlugin(const nlohmann::json& cfg) {
    return new RegionsPlugin(cfg);
}