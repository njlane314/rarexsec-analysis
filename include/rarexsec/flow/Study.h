#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include <algorithm>

#include "EventDisplayBuilder.h"
#include "PlotBuilders.h"
#include "SnapshotBuilder.h"
#include <rarexsec/plug/PipelineRunner.h>
#include <rarexsec/plug/PluginArgs.h>
#include <rarexsec/plug/PluginSpec.h>

namespace analysis::dsl {

struct RegionDef {
    std::string key;
    std::string label;
    std::string expr;
};

struct VarDef {
    std::string name;
    std::string branch;
    std::string label;
    std::string stratum{"channel_definitions"};
    std::vector<std::string> regions;
    nlohmann::json binning{{"n", 100}, {"min", 0.0}, {"max", 1.0}};

    explicit VarDef(std::string n)
        : name(std::move(n)), branch(name), label(name) {}

    VarDef &as(std::string b) {
        branch = std::move(b);
        return *this;
    }
    VarDef &titled(std::string l) {
        label = std::move(l);
        return *this;
    }
    VarDef &stratify(std::string s) {
        stratum = std::move(s);
        return *this;
    }
    VarDef &in(std::string r) {
        regions.push_back(std::move(r));
        return *this;
    }
    VarDef &bins_config(nlohmann::json b) {
        binning = std::move(b);
        return *this;
    }
    VarDef &bins(int n, double mn, double mx) {
        binning = {{"n", n}, {"min", mn}, {"max", mx}};
        return *this;
    }
};

class Study {
  public:
    explicit Study(std::string name) : name_(std::move(name)) {}

    Study &data(std::string samples_json) {
        samples_path_ = std::move(samples_json);
        return *this;
    }

    Study &region(std::string key, std::string expression,
                  std::string label = "") {
        if (label.empty())
            label = key;
        regions_.push_back(
            RegionDef{std::move(key), std::move(label), std::move(expression)});
        return *this;
    }

    Study &var(std::string variable_name) {
        variables_.emplace_back(std::move(variable_name));
        return *this;
    }

    Study &var(VarDef v) {
        variables_.push_back(std::move(v));
        return *this;
    }

    Study &plot(PlotDef p) {
        plots_.push_back(std::move(p));
        return *this;
    }
    Study &plot(const PerformanceBuilder &p) {
        perf_.push_back(p.to_json());
        return *this;
    }
    Study &plot(const CutFlowBuilder &c) {
        cutflow_.push_back(c.to_json());
        return *this;
    }
    Study &plot(const SurvivalBuilder &s) {
        survival_.push_back(s.to_json());
        return *this;
    }
    Study &display(const EventDisplayBuilder &ed) {
        displays_.push_back(ed.to_json());
        return *this;
    }
    Study &snapshot(const SnapshotBuilder &s) {
        snaps_.push_back(s.to_json());
        return *this;
    }

    void run(const std::string &out_root_path) const {
        PluginSpecList analysis_specs;
        PluginSpecList plot_specs;

        nlohmann::json regions = nlohmann::json::array();
        for (auto const &r : regions_) {
            regions.push_back({{"region_key", r.key},
                               {"label", r.label},
                               {"expression", r.expr}});
        }
        analysis_specs.push_back(
            {"RegionsPlugin", {{"analysis_configs", {{"regions", regions}}}}});

        if (!variables_.empty()) {
            nlohmann::json vars_cfg = nlohmann::json::array();
            for (auto const &v : variables_) {
                nlohmann::json r = PluginArgs::array();
                if (!v.regions.empty()) {
                    for (auto const &reg : v.regions)
                        r.push_back(reg);
                } else {
                    r.push_back(defaultRegionKey());
                }
                vars_cfg.push_back({{"name", v.name},
                                    {"branch", v.branch},
                                    {"label", v.label},
                                    {"stratum", v.stratum},
                                    {"regions", r},
                                    {"bins", v.binning}});
            }
            analysis_specs.push_back(
                {"VariablesPlugin",
                 {{"analysis_configs", {{"variables", vars_cfg}}}}});
        }

        for (auto const &p : plots_) {
            if (p.kind == "stack") {
                PluginArgs args{
                    {"plot_configs",
                     {{"plots",
                       nlohmann::json::array(
                           {{{"variable", p.variable},
                             {"region",
                              p.region.empty() ? defaultRegionKey() : p.region},
                             {"signal_group", p.signal_group},
                             {"logy", p.logy}}})}}}};
                plot_specs.push_back({"StackedHistogramPlugin", std::move(args)});
            } else if (p.kind == "roc") {
                PluginArgs args{
                    {"performance_plots",
                     nlohmann::json::array(
                         {{{"region",
                            p.region.empty() ? defaultRegionKey() : p.region},
                           {"channel_column", p.channel_column},
                           {"signal_group", p.signal_group},
                           {"variable", p.variable}}})}};
                plot_specs.push_back(
                    {"PerformancePlotPlugin", std::move(args)});
            }
        }

        if (!perf_.empty())
            plot_specs.push_back(
                {"PerformancePlotPlugin",
                 {{"plot_configs", {{"performance_plots", perf_}}}}});
        if (!cutflow_.empty())
            plot_specs.push_back({"CutFlowPlotPlugin",
                                  {{"plot_configs", {{"plots", cutflow_}}}}});
        if (!survival_.empty())
            plot_specs.push_back({"SignalCutFlowPlotPlugin",
                                  {{"plot_configs", {{"plots", survival_}}}}});
        if (!snaps_.empty())
            analysis_specs.push_back(
                {"SnapshotPlugin",
                 {{"analysis_configs", {{"snapshots", snaps_}}}}});
        if (!displays_.empty()) {
            auto displays = displays_;
            for (auto &ed : displays) {
                if (ed.contains("region")) {
                    const auto &rk = ed.at("region").get_ref<const std::string&>();
                    auto it = std::find_if(regions_.begin(), regions_.end(),
                                           [&](const RegionDef &r) { return r.key == rk; });
                    if (it != regions_.end())
                        ed["selection_expr"] = it->expr;
                }
            }
            plot_specs.push_back(
                {"EventDisplayPlugin",
                 {{"plot_configs", {{"event_displays", displays}}}}});
        }

        auto unique = [](PluginSpecList &v) {
            std::unordered_set<std::string> seen;
            PluginSpecList out;
            out.reserve(v.size());
            for (auto &s : v)
                if (seen.insert(s.id).second)
                    out.push_back(std::move(s));
            v.swap(out);
        };
        unique(analysis_specs);
        unique(plot_specs);

        PipelineRunner runner(std::move(analysis_specs), std::move(plot_specs));
        runner.run(samples_path_, out_root_path);
    }

  private:
    std::string defaultRegionKey() const {
        return regions_.empty() ? std::string{} : regions_.front().key;
    }

    std::string name_;
    std::string samples_path_;
    std::vector<RegionDef> regions_;
    std::vector<VarDef> variables_;
    std::vector<PlotDef> plots_;
    std::vector<nlohmann::json> perf_;
    std::vector<nlohmann::json> cutflow_;
    std::vector<nlohmann::json> survival_;
    std::vector<nlohmann::json> snaps_;
    std::vector<nlohmann::json> displays_;
};

}
