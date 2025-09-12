#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <ROOT/RConfig.h>
#include <ROOT/RDataFrame.hxx>

#include <rarexsec/core/SelectionQuery.h>
#include <rarexsec/core/SelectionRegistry.h>
#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/plot/DetectorDisplay.h>
#include <rarexsec/plot/SemanticDisplay.h>
#include <rarexsec/plug/IPlotPlugin.h>
#include <rarexsec/plug/PluginRegistry.h>
#include <rarexsec/utils/Logger.h>

namespace analysis {

class EventDisplayPlugin : public IPlotPlugin {
public:
  struct DisplayConfig {
    std::string sample;
    std::string region;
    SelectionQuery selection;
    std::optional<std::string> selection_expr;
    int n_events{1};
    int image_size{512};
    std::string image_format{"png"};
    std::filesystem::path output_directory{"./plots/event_displays"};
    std::vector<std::string> planes{"U", "V", "W"};
    std::string mode{"detector"};
    std::string file_pattern{"{plane}_{run}_{sub}_{evt}"};
    std::optional<unsigned> seed;
    std::optional<std::string> order_by;
    bool order_desc{true};
    std::string manifest_path;
    std::string combined_pdf;
  };

  EventDisplayPlugin(const PluginArgs &args, AnalysisDataLoader *loader)
      : loader_(loader) {
    const auto &cfg = args.plot_configs;
    if (!cfg.contains("event_displays") ||
        !cfg.at("event_displays").is_array()) {
      throw std::runtime_error("EventDisplayPlugin missing event_displays");
    }
    SelectionRegistry sel_reg;
    for (auto const &ed : cfg.at("event_displays")) {
      DisplayConfig dc;
      dc.sample = ed.at("sample").get<std::string>();
      dc.region = ed.value("region", std::string{});
      dc.n_events = ed.value("n_events", 1);
      dc.image_size = ed.value("image_size", 512);
      dc.image_format = ed.value("image_format", std::string{"png"});
      dc.output_directory =
          ed.value("output_directory", std::string{"./plots/event_displays"});
      dc.output_directory =
          std::filesystem::absolute(dc.output_directory).lexically_normal();
      dc.planes = ed.value("planes", std::vector<std::string>{"U", "V", "W"});
      dc.mode = ed.value("mode", std::string{"detector"});
      dc.file_pattern =
          ed.value("file_pattern", std::string{"{plane}_{run}_{sub}_{evt}"});
      if (ed.contains("seed"))
        dc.seed = ed.at("seed").get<unsigned>();
      if (ed.contains("order_by")) {
        dc.order_by = ed.at("order_by").get<std::string>();
        dc.order_desc = ed.value("order_desc", true);
      }
      dc.manifest_path = ed.value("manifest", std::string{});
      dc.combined_pdf = ed.value("combined_pdf", std::string{});
      if (ed.contains("selection_expr"))
        dc.selection_expr = ed.at("selection_expr").get<std::string>();
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
      log::info(
          "EventDisplayPlugin",
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
      log::error("EventDisplayPlugin::onPlot",
                 "No AnalysisDataLoader context provided");
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
      bool has_filter =
          filter.find_first_not_of(" \t\n\r") != std::string::npos;
      if (has_filter)
        df = df.Filter(filter);

      if (cfg.order_by) {
        log::warn("EventDisplayPlugin",
                  "order_by not implemented; proceeding without ordering");
      }

      auto limited = df.Range(static_cast<ULong64_t>(cfg.n_events));
      std::filesystem::path out_dir = cfg.output_directory / cfg.sample;

      // Ensure the output directory exists before processing events.  Using
      // std::filesystem avoids relying solely on ROOT's gSystem which may
      // fail silently on some platforms.
      std::error_code ec;
      std::filesystem::create_directories(out_dir, ec);
      if (ec) {
        log::error("EventDisplayPlugin",
                   "Failed to create output directory:", out_dir.string(),
                   ec.message());
      }

      nlohmann::json manifest = nlohmann::json::array();
      std::mutex manifest_mutex;
      std::atomic<size_t> saved{0};
      std::mutex pdf_mutex;

      bool use_combined_pdf = !cfg.combined_pdf.empty() &&
                              cfg.image_format == "pdf";
      std::filesystem::path combined_path;
      size_t total_pages =
          static_cast<size_t>(cfg.n_events) * cfg.planes.size();
      if (use_combined_pdf)
        combined_path = out_dir / cfg.combined_pdf;

      const std::vector<std::string> cols{"run",
                                          "sub",
                                          "evt",
                                          "event_detector_image_u",
                                          "event_detector_image_v",
                                          "event_detector_image_w",
                                          "semantic_image_u",
                                          "semantic_image_v",
                                          "semantic_image_w"};

      auto cfg_copy = cfg; // capture by value

      limited.ForeachSlot(
          [&](unsigned /*slot*/, int run, int sub, int evt,
              const std::vector<float> &det_u, const std::vector<float> &det_v,
              const std::vector<float> &det_w, const std::vector<int> &sem_u,
              const std::vector<int> &sem_v, const std::vector<int> &sem_w) {
            auto formatTag = [](std::string pattern, const std::string &plane,
                                int r, int s, int e) {
              auto replace_all = [](std::string &str, const std::string &from,
                                    const std::string &to) {
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

            auto process = [&](const std::string &plane,
                               const std::vector<float> &det,
                               const std::vector<int> &sem) {
              std::string tag =
                  formatTag(cfg_copy.file_pattern, plane, run, sub, evt);
              std::string title_prefix =
                  cfg_copy.mode == "semantic" ? "Semantic Image, Plane "
                                               : "Detector Image, Plane ";
              std::string title =
                  title_prefix + plane + " - Run " + std::to_string(run) +
                  ", Subrun " + std::to_string(sub) + ", Event " +
                  std::to_string(evt);

              std::string out_file_record;
              std::string save_target;

              if (use_combined_pdf) {
                size_t idx = saved.fetch_add(1);
                bool first = (idx == 0);
                bool last = (idx == total_pages - 1);
                save_target = combined_path.string();
                if (first)
                  save_target += "(";
                else if (last)
                  save_target += ")";
                if (cfg_copy.mode == "semantic") {
                  SemanticDisplay s(tag, title, sem, cfg_copy.image_size,
                                    out_dir.string());
                  std::lock_guard<std::mutex> lock(pdf_mutex);
                  s.drawAndSave("pdf", save_target);
                } else {
                  DetectorDisplay d(tag, title, det, cfg_copy.image_size,
                                    out_dir.string());
                  std::lock_guard<std::mutex> lock(pdf_mutex);
                  d.drawAndSave("pdf", save_target);
                }
                out_file_record = combined_path.string();
              } else {
                auto out_file = out_dir / (tag + "." + cfg_copy.image_format);
                if (cfg_copy.mode == "semantic") {
                  SemanticDisplay s(tag, title, sem, cfg_copy.image_size,
                                    out_dir.string());
                  s.drawAndSave(cfg_copy.image_format, out_file.string());
                } else {
                  DetectorDisplay d(tag, title, det, cfg_copy.image_size,
                                    out_dir.string());
                  d.drawAndSave(cfg_copy.image_format, out_file.string());
                }
                save_target = out_file.string();
                out_file_record = out_file.string();
                saved.fetch_add(1);
              }

              log::info("EventDisplayPlugin", "Saved event display:",
                        save_target);
              if (!cfg_copy.manifest_path.empty()) {
                std::lock_guard<std::mutex> lock(manifest_mutex);
                manifest.push_back({{"run", run},
                                    {"sub", sub},
                                    {"evt", evt},
                                    {"plane", plane},
                                    {"file", out_file_record}});
              }
            };

            for (auto const &plane : cfg_copy.planes) {
              if (plane == "U")
                process("U", det_u, sem_u);
              else if (plane == "V")
                process("V", det_v, sem_v);
              else if (plane == "W")
                process("W", det_w, sem_w);
            }
          },
          cols);

      if (!cfg.manifest_path.empty()) {
        std::ofstream ofs(cfg.manifest_path);
        ofs << manifest.dump(2);
        log::info("EventDisplayPlugin",
                  "Wrote event display manifest:", cfg.manifest_path);
      }

      if (saved == 0) {
        log::warn("EventDisplayPlugin",
                  "No events matched selection for sample:", cfg.sample);
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
extern "C" analysis::IPlotPlugin *
createEventDisplayPlugin(const analysis::PluginArgs &args) {
  return new analysis::EventDisplayPlugin(
      args, analysis::EventDisplayPlugin::legacyLoader());
}
extern "C" analysis::IPlotPlugin *
createPlotPlugin(const analysis::PluginArgs &args) {
  return createEventDisplayPlugin(args);
}
extern "C" void setPluginContext(analysis::AnalysisDataLoader *loader) {
  analysis::EventDisplayPlugin::setLegacyLoader(loader);
}
#endif
