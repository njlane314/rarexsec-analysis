#include "EventDisplayPlugin.h"

using namespace analysis;

AnalysisDataLoader* EventDisplayPlugin::loader_ = nullptr;

EventDisplayPlugin::EventDisplayPlugin(const nlohmann::json& cfg) {
    if (!cfg.contains("event_displays") || !cfg.at("event_displays").is_array()) {
        throw std::runtime_error("EventDisplayPlugin missing event_displays");
    }
    for (auto const& ed : cfg.at("event_displays")) {
        DisplayConfig dc;
        dc.sample          = ed.at("sample").get<std::string>();
        dc.region          = ed.value("region", std::string{});
        dc.n_events        = ed.value("n_events", 1);
        dc.pdf_name        = ed.value("pdf_name", std::string{"event_displays.pdf"});
        dc.image_size      = ed.value("image_size", 800);
        dc.output_directory= ed.value("output_directory", std::string{"plots"});
        configs_.push_back(std::move(dc));
    }
}

void EventDisplayPlugin::onInitialisation(AnalysisDefinition& def, const SelectionRegistry&) {
    for (auto& cfg : configs_) {
        if (!cfg.region.empty()) {
            try {
                RegionKey rkey{cfg.region};
                cfg.selection = def.region(rkey).selection();
            } catch (const std::exception&) {
                log::error("EventDisplayPlugin::onInitialisation", "Unknown region:", cfg.region);
            }
        }
    }
}

void EventDisplayPlugin::onFinalisation(const RegionAnalysisMap&) {
    if (!loader_) {
        log::error("EventDisplayPlugin::onFinalisation", "No AnalysisDataLoader context provided");
        return;
    }
    for (auto const& cfg : configs_) {
        PlotCatalog catalog(*loader_, cfg.image_size, cfg.output_directory);
        catalog.generateRandomEventDisplays(cfg.sample, cfg.selection, cfg.n_events, cfg.pdf_name);
    }
}

extern "C" IAnalysisPlugin* createPlugin(const nlohmann::json& cfg) {
    return new EventDisplayPlugin(cfg);
}

extern "C" void setPluginContext(AnalysisDataLoader* loader) {
    EventDisplayPlugin::loader_ = loader;
}
