#include "PresetRegistry.h"
#include "PluginSpec.h"
#include <nlohmann/json.hpp>

using namespace analysis;

// Presets for plot-specific configuration.
// Configures stacked histogram plots stratified by the inclusive category scheme.
ANALYSIS_REGISTER_PRESET(STACKED_PLOTS, Target::Plot,
  ([](const PluginArgs&) -> PluginSpecList {
    nlohmann::json plot = {{"category_column", "channel_definitions"}};
    PluginArgs args{{"plot_configs", {{"plots", nlohmann::json::array({plot})}}}};
    return {{"StackedHistogramPlugin", args}};
  })
)

// Preset to configure the EventDisplay plugin with a single display request.
// Values are taken from the provided variables or fall back to sensible
// defaults matching the plugin's own choices.
ANALYSIS_REGISTER_PRESET(EVENT_DISPLAY, Target::Plot,
  ([](const PluginArgs& vars) -> PluginSpecList {
    auto cfg = vars.plot_configs;
    nlohmann::json ed = {
      {"sample", cfg.value("sample", std::string{})},
      {"region", cfg.value("region", std::string{})},
      {"n_events", cfg.value("n_events", 1)},
      {"image_size", cfg.value("image_size", 800)},
      {"output_directory",
       cfg.value("output_directory", std::string{"./plots/event_displays"})}
    };
    PluginArgs args{{"plot_configs",
                     {{"event_displays", nlohmann::json::array({ed})}}}};
    return {{"EventDisplayPlugin", args}};
  })
)

// Preset to configure detector event displays for background events.  Uses the
// inclusive MC sample by default and writes images to a dedicated directory.
ANALYSIS_REGISTER_PRESET(BACKGROUND_EVENT_DISPLAY, Target::Plot,
  ([](const PluginArgs& vars) -> PluginSpecList {
    auto cfg = vars.plot_configs;
    nlohmann::json ed = {
      {"sample", cfg.value("sample", std::string{"mc_inclusive_run1_fhc"})},
      {"region", cfg.value("region", std::string{})},
      {"n_events", cfg.value("n_events", 1)},
      {"image_size", cfg.value("image_size", 800)},
      {"output_directory",
       cfg.value("output_directory", std::string{"./plots/background_event_displays"})}
    };
    PluginArgs args{{"plot_configs",
                     {{"event_displays", nlohmann::json::array({ed})}}}};
    return {{"EventDisplayPlugin", args}};
  })
)

// Preset to configure detector event displays for signal events.  Defaults to
// the strangeness-enriched MC sample and saves images separately.
ANALYSIS_REGISTER_PRESET(SIGNAL_EVENT_DISPLAY, Target::Plot,
  ([](const PluginArgs& vars) -> PluginSpecList {
    auto cfg = vars.plot_configs;
    nlohmann::json ed = {
      {"sample", cfg.value("sample", std::string{"mc_strangeness_run1_fhc"})},
      {"region", cfg.value("region", std::string{})},
      {"n_events", cfg.value("n_events", 1)},
      {"image_size", cfg.value("image_size", 800)},
      {"output_directory",
       cfg.value("output_directory", std::string{"./plots/signal_event_displays"})}
    };
    PluginArgs args{{"plot_configs",
                     {{"event_displays", nlohmann::json::array({ed})}}}};
    return {{"EventDisplayPlugin", args}};
  })
)

// Preset to configure the CutFlow plot plugin with a single cut flow request.
// Defaults target the inclusive strange channel scheme and write output to a
// dedicated directory.
ANALYSIS_REGISTER_PRESET(CUT_FLOW_PLOT, Target::Plot,
  ([](const PluginArgs& vars) -> PluginSpecList {
    auto cfg = vars.plot_configs;
    nlohmann::json plot = {
      {"selection_rule", cfg.value("selection_rule", std::string{})},
      {"region", cfg.value("region", std::string{})},
      {"signal_group",
       cfg.value("signal_group", std::string{"inclusive_strange_channels"})},
      {"channel_column",
       cfg.value("channel_column", std::string{"channel_definitions"})},
      {"initial_label", cfg.value("initial_label", std::string{"All events"})},
      {"plot_name", cfg.value("plot_name", std::string{"cut_flow"})},
      {"output_directory",
       cfg.value("output_directory", std::string{"./plots/cut_flow"})},
      {"log_y", cfg.value("log_y", false)},
      {"clauses", cfg.value("clauses", nlohmann::json::array())}
    };
    PluginArgs args{{"plot_configs",
                     {{"plots", nlohmann::json::array({plot})}}}};
    return {{"CutFlowPlotPlugin", args}};
  })
)

