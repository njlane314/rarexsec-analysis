#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

#include <ROOT/RConfig.h>
#include <ROOT/RDataFrame.hxx>

#include <rarexsec/plug/PluginRegistry.h>
#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/utils/Logger.h>
#include <rarexsec/plug/IPlotPlugin.h>
#include <rarexsec/plot/DetectorDisplay.h>
#include <rarexsec/plot/SemanticDisplay.h>
#include <rarexsec/core/SelectionQuery.h>
#include <rarexsec/core/SelectionRegistry.h>

namespace analysis {

class EventDisplayPlugin : public IPlotPlugin {
  public:
    struct DisplayConfig {
        std::string sample;
        std::string region;
        SelectionQuery selection;
        std::optional<std::string> selection_expr;
        int n_events{1};
        int image_size{800};
        std::filesystem::path output_directory{"./plots/event_displays"};
        std::vector<std::string> planes{"U","V","W"};
        std::string mode{"detector"};
        std::string file_pattern{"{plane}_{run}_{sub}_{evt}"};
        std::optional<unsigned> seed;
        std::optional<std::string> order_by;
        bool order_desc{true};
        std::string manifest_path;
    };

    EventDisplayPlugin(const PluginArgs &args, AnalysisDataLoader *loader) : loader_(loader) {
        const auto &cfg = args.plot_configs;
        if (!cfg.contains("event_displays") || !cfg.at("event_displays").is_array()) {
            throw std::runtime_error("EventDisplayPlugin missing event_displays");
        }
        SelectionRegistry sel_reg;
        for (auto const &ed : cfg.at("event_displays")) {
            DisplayConfig dc;
            dc.sample = ed.at("sample").get<std::string>();
            dc.region = ed.value("region", std::string{});
            dc.n_events = ed.value("n_events", 1);
            dc.image_size = ed.value("image_size", 800);
            dc.output_directory = ed.value("output_directory", std::string{"./plots/event_displays"});
            dc.output_directory = std::filesystem::absolute(dc.output_directory).lexically_normal();
            dc.planes = ed.value("planes", std::vector<std::string>{"U","V","W"});
            dc.mode = ed.value("mode", std::string{"detector"});
            dc.file_pattern = ed.value("file_pattern", std::string{"{plane}_{run}_{sub}_{evt}"});
            if (ed.contains("seed")) dc.seed = ed.at("seed").get<unsigned>();
            if (ed.contains("order_by")) {
                dc.order_by = ed.at("order_by").get<std::string>();
                dc.order_desc = ed.value("order_desc", true);
            }
            dc.manifest_path = ed.value("manifest", std::string{});
            if (ed.contains("selection_expr")) dc.selection_expr = ed.at("selection_expr").get<std::string>();
            if (!dc.region.empty()) {
                try {
                    dc.selection = sel_reg.get(dc.region);
                } catch (const std::exception &) {
                    log::error("EventDisplayPlugin", "Unknown region:", dc.region);
                }
            }
            if (dc.selection_expr) {
                dc.selection = SelectionQuery(*dc.selection_expr);
            }
            configs_.push_back(std::move(dc));
        }
    }

    void onPlot(const AnalysisResult &) override {
#if defined(R__HAS_IMPLICITMT)
        if (ROOT::IsImplicitMTEnabled() && ROOT::GetThreadPoolSize() <= 1) {
            ROOT::DisableImplicitMT();
            log::info("EventDisplayPlugin",
                      "Implicit multithreading not supported; running single-threaded.");
        }
#else
        if (ROOT::IsImplicitMTEnabled()) {
            ROOT::DisableImplicitMT();
            log::info("EventDisplayPlugin",
                      "ROOT built without multithreading; running single-threaded.");
        }
#endif
        if (!loader_) {
            log::error("EventDisplayPlugin::onPlot", "No AnalysisDataLoader context provided");
            return;
        }
        for (auto const &cfg : configs_) {
            auto &frames = loader_->getSampleFrames();
            auto skey = SampleKey{cfg.sample};
            auto it = frames.find(skey);
            if (it == frames.end()) {
                log::error("EventDisplayPlugin", "Unknown sample:", cfg.sample);
                continue;
            }
            auto &sample = it->second;
            auto df = sample.nominal_node_;

            std::string filter = cfg.selection.str();
            bool has_filter = filter.find_first_not_of(" \t\n\r") != std::string::npos;
            if (has_filter)
                df = df.Filter(filter);

            if (cfg.order_by) {
                log::warn("EventDisplayPlugin", "order_by not implemented; proceeding without ordering");
            }

            auto limited = df.Range(static_cast<ULong64_t>(cfg.n_events));
            std::filesystem::path out_dir = cfg.output_directory / cfg.sample;

            nlohmann::json manifest = nlohmann::json::array();
            std::mutex manifest_mutex;

            const std::vector<std::string> cols{
                "run","sub","evt",
                "event_detector_image_u","event_detector_image_v","event_detector_image_w",
                "semantic_image_u","semantic_image_v","semantic_image_w"
            };

            auto cfg_copy = cfg; // capture by value

            limited.ForeachSlot([&](unsigned /*slot*/, int run, int sub, int evt,
                                     const std::vector<float>& det_u,
                                     const std::vector<float>& det_v,
                                     const std::vector<float>& det_w,
                                     const std::vector<int>&   sem_u,
                                     const std::vector<int>&   sem_v,
                                     const std::vector<int>&   sem_w){

                auto formatTag = [](std::string pattern, const std::string& plane, int r, int s, int e){
                    auto replace_all = [](std::string &str, const std::string &from, const std::string &to){
                        size_t pos = 0;
                        while ((pos = str.find(from, pos)) != std::string::npos) {
                            str.replace(pos, from.size(), to);
                            pos += to.size();
                        }
                    };
                    replace_all(pattern, "{plane}", plane);
                    replace_all(pattern, "{run}", std::to_string(r));
                    replace_all(pattern, "{sub}", std::to_string(s));
                    replace_all(pattern, "{evt}", std::to_string(e));
                    return pattern;
                };

                auto process = [&](const std::string& plane,
                                   const std::vector<float>& det,
                                   const std::vector<int>& sem){
                    std::string tag = formatTag(cfg_copy.file_pattern, plane, run, sub, evt);
                    auto out_file = out_dir / (tag + ".png");
                    if (cfg_copy.mode == "semantic") {
                        SemanticDisplay s(tag, sem, cfg_copy.image_size, out_dir.string());
                        s.drawAndSave();
                    } else {
                        DetectorDisplay d(tag, det, cfg_copy.image_size, out_dir.string());
                        d.drawAndSave();
                    }
                    log::info("EventDisplayPlugin", "Saved event display:", out_file.string());
                    if (!cfg_copy.manifest_path.empty()) {
                        std::lock_guard<std::mutex> lock(manifest_mutex);
                        manifest.push_back({{"run",run},{"sub",sub},{"evt",evt},{"plane",plane},{"file",out_file.string()}});
                    }
                };

                for (auto const& plane : cfg_copy.planes) {
                    if (plane == "U") process("U", det_u, sem_u);
                    else if (plane == "V") process("V", det_v, sem_v);
                    else if (plane == "W") process("W", det_w, sem_w);
                }
            }, cols);

            if (!cfg.manifest_path.empty()) {
                std::ofstream ofs(cfg.manifest_path);
                ofs << manifest.dump(2);
                log::info("EventDisplayPlugin", "Wrote event display manifest:", cfg.manifest_path);
            }
        }
    }

    static void setLegacyLoader(AnalysisDataLoader *ldr) { legacy_loader_ = ldr; }
    static AnalysisDataLoader *legacyLoader() { return legacy_loader_; }

  private:
    std::vector<DisplayConfig> configs_;
    AnalysisDataLoader *loader_;
    inline static AnalysisDataLoader *legacy_loader_ = nullptr;
};

} // namespace analysis

ANALYSIS_REGISTER_PLUGIN(analysis::IPlotPlugin, analysis::AnalysisDataLoader,
                         "EventDisplayPlugin", analysis::EventDisplayPlugin)

#ifdef BUILD_PLUGIN
extern "C" analysis::IPlotPlugin *createEventDisplayPlugin(
    const analysis::PluginArgs &args) {
    return new analysis::EventDisplayPlugin(args,
                                           analysis::EventDisplayPlugin::legacyLoader());
}
extern "C" analysis::IPlotPlugin *createPlotPlugin(const analysis::PluginArgs &args) {
    return createEventDisplayPlugin(args);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
    analysis::EventDisplayPlugin::setLegacyLoader(loader);
}
#endif
