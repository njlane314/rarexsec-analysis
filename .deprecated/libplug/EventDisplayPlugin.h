#ifndef EVENTDISPLAYPLUGIN_H
#define EVENTDISPLAYPLUGIN_H

#include <algorithm>
#include <array>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisLogger.h"
#include "EventDisplay.h"
#include "IAnalysisPlugin.h"
#include "Selection.h"

namespace analysis {

class EventDisplayPlugin : public IAnalysisPlugin {
  public:
    struct DisplayConfig {
        std::string sample;
        std::string region;
        Selection selection;
        int n_events{1};
        int image_size{800};
        std::filesystem::path output_directory{"./plots/event_displays"};
    };

    inline explicit EventDisplayPlugin(const nlohmann::json &cfg) {
        if (!cfg.contains("event_displays") || !cfg.at("event_displays").is_array()) {
            throw std::runtime_error("EventDisplayPlugin missing event_displays");
        }
        for (auto const &ed : cfg.at("event_displays")) {
            DisplayConfig dc;
            dc.sample = ed.at("sample").get<std::string>();
            dc.region = ed.value("region", std::string{});
            dc.n_events = ed.value("n_events", 1);
            dc.image_size = ed.value("image_size", 800);
            dc.output_directory = ed.value("output_directory", std::string{"./plots/event_displays"});
            dc.output_directory = std::filesystem::absolute(dc.output_directory).lexically_normal();
            configs_.push_back(std::move(dc));
        }
    }

    inline void onInitialisation(AnalysisDefinition &def, const SelectionRegistry &) override {
        for (auto &cfg : configs_) {
            if (!cfg.region.empty()) {
                try {
                    RegionKey rkey{cfg.region};
                    cfg.selection = def.region(rkey).selection();
                } catch (const std::exception &) {
                    log::error("EventDisplayPlugin::onInitialisation", "Unknown region:", cfg.region);
                }
            }
        }
    }

    inline void onPreSampleProcessing(const SampleKey &, const RegionKey &, const RunConfig &) override {}
    inline void onPostSampleProcessing(const SampleKey &, const RegionKey &, const RegionAnalysisMap &) override {}

    inline void onFinalisation(const RegionAnalysisMap &) override {
        if (!loader_) {
            log::error("EventDisplayPlugin::onFinalisation", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &cfg : configs_) {
            auto &sample = loader_->getSampleFrames().at(SampleKey{cfg.sample});
            auto df = sample.nominal_node_;

            std::string filter = cfg.selection.str();
            bool has_filter = filter.find_first_not_of(" \t\n\r") != std::string::npos;
            if (has_filter)
                df = df.Filter(filter);

            auto runs = df.Take<int>("run").GetValue();
            auto subs = df.Take<int>("sub").GetValue();
            auto evts = df.Take<int>("evt").GetValue();

            size_t n = std::min<size_t>(cfg.n_events, runs.size());
            std::filesystem::path out_dir = cfg.output_directory / cfg.sample;

            for (size_t i = 0; i < n; ++i) {
                int run = runs[i];
                int sub = subs[i];
                int evt = evts[i];
                std::string expr = "run == " + std::to_string(run) + "&& sub == " + std::to_string(sub) +
                                   "&& evt == " + std::to_string(evt);
                auto edf = df.Filter(expr);

                auto det_u_vec = edf.Take<std::vector<float>>("event_detector_image_u").GetValue();
                auto det_v_vec = edf.Take<std::vector<float>>("event_detector_image_v").GetValue();
                auto det_w_vec = edf.Take<std::vector<float>>("event_detector_image_w").GetValue();

                auto sem_u_vec = edf.Take<std::vector<int>>("semantic_image_u").GetValue();
                auto sem_v_vec = edf.Take<std::vector<int>>("semantic_image_v").GetValue();
                auto sem_w_vec = edf.Take<std::vector<int>>("semantic_image_w").GetValue();

                if (det_u_vec.empty() || det_v_vec.empty() || det_w_vec.empty())
                    continue;
                if (sem_u_vec.empty() || sem_v_vec.empty() || sem_w_vec.empty())
                    continue;

                std::array<std::string, 3> planes = {"U", "V", "W"};
                for (size_t p = 0; p < planes.size(); ++p) {
                    std::string tag = planes[p] + std::string("_") + std::to_string(run) + "_" + std::to_string(sub) +
                                      "_" + std::to_string(evt);
                    std::vector<float> det_data = p == 0 ? det_u_vec[0] : (p == 1 ? det_v_vec[0] : det_w_vec[0]);
                    std::vector<int> sem_data = p == 0 ? sem_u_vec[0] : (p == 1 ? sem_v_vec[0] : sem_w_vec[0]);

                    log::info("EventDisplayPlugin", "Generating", tag, "display");

                    DetectorDisplayPlot det_disp(tag, det_data, cfg.image_size, out_dir.string());
                    det_disp.drawAndSave();

                    SemanticDisplayPlot sem_disp(tag, sem_data, cfg.image_size, out_dir.string());
                    sem_disp.drawAndSave();
                }
            }
        }
    }

    inline static void setLoader(AnalysisDataLoader *loader) { loader_ = loader; }

  private:
    std::vector<DisplayConfig> configs_;
    inline static AnalysisDataLoader *loader_ = nullptr;
};

} // namespace analysis

#ifdef BUILD_PLUGIN
extern "C" analysis::IAnalysisPlugin *createPlugin(const nlohmann::json &cfg) {
    return new analysis::EventDisplayPlugin(cfg);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::EventDisplayPlugin::setLoader(loader);
}
#endif

#endif
