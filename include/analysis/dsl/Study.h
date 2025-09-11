#pragma once
#include <string>
#include <vector>
#include <utility>
#include <nlohmann/json.hpp>

#include "PipelineBuilder.h"
#include "PipelineRunner.h"
#include "PresetRegistry.h"
#include "Display.h"
#include "Plots.h"
#include "Snapshot.h"

namespace analysis::dsl {

struct RegionDef {
  std::string key;
  std::string label;
  std::string expr;
};

class Study {
public:
  explicit Study(std::string name) : name_(std::move(name)) {}

  Study& data(std::string samples_json) {
    samples_path_ = std::move(samples_json);
    return *this;
  }

  Study& region(std::string key, std::string expression, std::string label="") {
    if (label.empty()) label = key;
    regions_.push_back(RegionDef{std::move(key), std::move(label), std::move(expression)});
    return *this;
  }

  Study& var(std::string variable_name) {
    variables_.push_back(std::move(variable_name));
    return *this;
  }

  Study& plot(PlotDef p) { plots_.push_back(std::move(p)); return *this; }
  Study& plot(const PerformanceBuilder& p){ perf_.push_back(p.to_json()); return *this; }
  Study& plot(const CutFlowBuilder& c){ cutflow_.push_back(c.to_json()); return *this; }
  Study& display(const EventDisplayBuilder& ed){ displays_.push_back(ed.to_json()); return *this; }
  Study& snapshot(const SnapshotBuilder& s){ snaps_.push_back(s.to_json()); return *this; }

  void run(const std::string& out_root_path) const {
    AnalysisPluginHost a_host;
    PlotPluginHost p_host;
    PipelineBuilder b(a_host, p_host);

    nlohmann::json regions = nlohmann::json::array();
    for (auto const& r : regions_) {
      regions.push_back({
        {"region_key", r.key},
        {"label", r.label},
        {"expression", r.expr}
      });
    }
    PluginArgs region_args{{"analysis_configs", {{"regions", regions}}}};
    b.add(Target::Analysis, "RegionsPlugin", region_args);

    for (auto const& v : variables_) {
      b.variable("VARIABLE:" + v,
                 {{"analysis_configs", {{"region", defaultRegionKey()}}}});
    }

    for (auto const& p : plots_) {
      if (p.kind == "stack") {
        PluginArgs args{{"plot_configs", {{"plots", nlohmann::json::array({{
            {"variable", p.variable},
            {"region",   p.region.empty() ? defaultRegionKey() : p.region},
            {"signal_group", p.signal_group},
            {"logy", p.logy}
        }})}}}};
        b.add(Target::Plot, "StackedPlotPlugin", std::move(args));
      } else if (p.kind == "roc") {
        PluginArgs args{{"performance_plots", nlohmann::json::array({{
            {"region",         p.region.empty() ? defaultRegionKey() : p.region},
            {"channel_column", p.channel_column},
            {"signal_group",   p.signal_group},
            {"variable",       p.variable}
        }})}};
        b.add(Target::Plot, "PerformancePlotPlugin", std::move(args));
      }
    }

    if (!perf_.empty()) {
      PluginArgs args{{"plot_configs", {{"performance_plots", perf_}}}};
      b.add(Target::Plot, "PerformancePlotPlugin", args);
    }
    if (!cutflow_.empty()) {
      PluginArgs args{{"plot_configs", {{"plots", cutflow_}}}};
      b.add(Target::Plot, "CutFlowPlotPlugin", args);
    }
    if (!snaps_.empty()) {
      PluginArgs args{{"analysis_configs", {{"snapshots", snaps_}}}};
      b.add(Target::Analysis, "SnapshotPlugin", args);
    }

    if (!displays_.empty()) {
      PluginArgs args{{"plot_configs", {{"event_displays", displays_}}}};
      b.add(Target::Plot, "EventDisplayPlugin", args);
    }

    b.uniqueById();

    PipelineRunner runner(b.analysisSpecs(), b.plotSpecs());
    runner.run(samples_path_, out_root_path);
  }

private:
  std::string defaultRegionKey() const {
    return regions_.empty() ? std::string{} : regions_.front().key;
  }

  std::string name_;
  std::string samples_path_;
  std::vector<RegionDef> regions_;
  std::vector<std::string> variables_;
  std::vector<PlotDef> plots_;
  std::vector<nlohmann::json> perf_;
  std::vector<nlohmann::json> cutflow_;
  std::vector<nlohmann::json> snaps_;
  std::vector<nlohmann::json> displays_;
};

} // namespace analysis::dsl

