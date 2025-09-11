#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace analysis::dsl {

struct PlotDef {
  std::string kind;             // "stack" | "roc" | ...
  std::string variable;
  std::string region;           // optional default
  std::string signal_group;     // optional
  std::string channel_column;   // ROC/perf
  bool logy = false;

  PlotDef& in(std::string r){ region = std::move(r); return *this; }
  PlotDef& signal(std::string s){ signal_group = std::move(s); return *this; }
  PlotDef& channel(std::string c){ channel_column = std::move(c); return *this; }
  PlotDef& logY(){ logy = true; return *this; }
};

inline PlotDef stack(std::string v){ return PlotDef{"stack", std::move(v)}; }
inline PlotDef roc(std::string v){ return PlotDef{"roc", std::move(v)}; }

// Cut direction helpers for Performance plots
namespace dir {
  static constexpr const char* gt = "GreaterThan";
  static constexpr const char* lt = "LessThan";
}

// -------- Performance (PerformancePlotPlugin) -----------------------------
class PerformanceBuilder {
public:
  explicit PerformanceBuilder(std::string var): variable_(std::move(var)) {}

  PerformanceBuilder& in(std::string region){ region_ = std::move(region); return *this; }
  PerformanceBuilder& channel(std::string col){ channel_column_ = std::move(col); return *this; }
  PerformanceBuilder& signal(std::string grp){ signal_group_ = std::move(grp); return *this; }
  PerformanceBuilder& bins(int n, double mn, double mx){ n_bins_=n; min_=mn; max_=mx; return *this; }
  PerformanceBuilder& cut(const char* d){ cut_dir_ = d; return *this; }
  PerformanceBuilder& name(std::string n){ plot_name_ = std::move(n); return *this; }
  PerformanceBuilder& out(std::string d){ out_dir_ = std::move(d); return *this; }
  PerformanceBuilder& where_all(std::vector<std::string> clauses){ clauses_ = std::move(clauses); return *this; }

  nlohmann::json to_json() const {
    nlohmann::json j{
      {"region", region_},
      {"channel_column", channel_column_},
      {"signal_group",   signal_group_},
      {"variable",       variable_},
      {"output_directory", out_dir_},
      {"plot_name",        plot_name_},
      {"n_bins", n_bins_}, {"min", min_}, {"max", max_},
      {"cut_direction",    cut_dir_}
    };
    if (!clauses_.empty()) j["clauses"] = clauses_;
    return j;
  }
private:
  std::string region_, channel_column_, signal_group_, variable_;
  std::string out_dir_{"plots"}, plot_name_{"performance_plot"};
  int n_bins_{100}; double min_{0.0}, max_{1.0};
  const char* cut_dir_ = dir::gt;
  std::vector<std::string> clauses_;
};
inline PerformanceBuilder perf(std::string variable){ return PerformanceBuilder(std::move(variable)); }

// -------- CutFlow (CutFlowPlotPlugin) -------------------------------------
class CutFlowBuilder {
public:
  CutFlowBuilder& rule(std::string r){ selection_rule_ = std::move(r); return *this; }
  CutFlowBuilder& in(std::string r){ region_ = std::move(r); return *this; }
  CutFlowBuilder& signal(std::string g){ signal_group_ = std::move(g); return *this; }
  CutFlowBuilder& channel(std::string c){ channel_column_ = std::move(c); return *this; }
  CutFlowBuilder& initial(std::string lab){ initial_label_ = std::move(lab); return *this; }
  CutFlowBuilder& steps(std::vector<std::string> c){ clauses_ = std::move(c); return *this; }
  CutFlowBuilder& name(std::string n){ plot_name_ = std::move(n); return *this; }
  CutFlowBuilder& logY(bool v=true){ log_y_ = v; return *this; }
  CutFlowBuilder& out(std::string d){ out_dir_ = std::move(d); return *this; }

  nlohmann::json to_json() const {
    nlohmann::json j{
      {"selection_rule", selection_rule_},
      {"region", region_},
      {"signal_group", signal_group_},
      {"channel_column", channel_column_},
      {"initial_label", initial_label_},
      {"plot_name", plot_name_},
      {"output_directory", out_dir_},
      {"log_y", log_y_},
    };
    if (!clauses_.empty()) j["clauses"] = clauses_;
    return j;
  }
private:
  std::string selection_rule_, region_, signal_group_, channel_column_;
  std::string initial_label_{"All events"}, plot_name_{"cutflow"};
  std::string out_dir_{"plots"};
  bool log_y_{false};
  std::vector<std::string> clauses_;
};
inline CutFlowBuilder cutflow(){ return {}; }

} // namespace analysis::dsl
