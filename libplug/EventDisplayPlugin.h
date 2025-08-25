#ifndef EVENTDISPLAYPLUGIN_H
#define EVENTDISPLAYPLUGIN_H

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AnalysisLogger.h"
#include "IAnalysisPlugin.h"
#include "PlotCatalog.h"
#include "Selection.h"

namespace analysis {

class EventDisplayPlugin : public IAnalysisPlugin {
  public:
    struct DisplayConfig {
        std::string sample;
        std::string region;
        Selection selection;
        int n_events{1};
        std::string pdf_name{"event_displays.pdf"};
        int image_size{800};
        std::string output_directory{"plots"};
    };

    inline explicit EventDisplayPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("event_displays") ||
            !cfg.at("event_displays").is_array()) {
            throw std::runtime_error(
                "EventDisplayPlugin missing event_displays");
        }
        for (auto const &ed : cfg.at("event_displays")) {
            DisplayConfig dc;
            dc.sample = ed.at("sample").get<std::string>();
            dc.region = ed.value("region", std::string{});
            dc.n_events = ed.value("n_events", 1);
            dc.pdf_name =
                ed.value("pdf_name", std::string{"event_displays.pdf"});
            dc.image_size = ed.value("image_size", 800);
            dc.output_directory =
                ed.value("output_directory", std::string{"plots"});
            configs_.push_back(std::move(dc));
        }
    }

    inline void onInitialisation(AnalysisDefinition &def,
                                 const SelectionRegistry &) override {
        for (auto &cfg : configs_) {
            if (!cfg.region.empty()) {
                try {
                    RegionKey rkey{cfg.region};
                    cfg.selection = def.region(rkey).selection();
                } catch (const std::exception &) {
                    log::error("EventDisplayPlugin::onInitialisation",
                               "Unknown region:", cfg.region);
                }
            }
        }
    }

    inline void onPreSampleProcessing(const SampleKey &, const RegionKey &,
                                      const RunConfig &) override {}
    inline void onPostSampleProcessing(const SampleKey &, const RegionKey &,
                                       const RegionAnalysisMap &) override {}

    inline void onFinalisation(const RegionAnalysisMap &) override {
        if (!loader_) {
            log::error("EventDisplayPlugin::onFinalisation",
                       "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &cfg : configs_) {
            PlotCatalog catalog(*loader_, cfg.image_size, cfg.output_directory);
            auto produced = catalog.generateRandomEventDisplays(
                cfg.sample, cfg.selection, cfg.n_events, cfg.pdf_name);
            if (produced > 0) {
                log::info("EventDisplayPlugin::onFinalisation",
                          "Saved", produced, "event displays to",
                          cfg.output_directory + "/" + cfg.pdf_name);
            } else {
                log::warn("EventDisplayPlugin::onFinalisation",
                          "No events found for", cfg.sample,
                          "in region", cfg.region);
            }
        }
    }

    inline static void setLoader(AnalysisDataLoader *loader) {
        loader_ = loader;
    }

  private:
    std::vector<DisplayConfig> configs_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

} // namespace analysis

#endif
