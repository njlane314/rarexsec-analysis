#include "PresetRegistry.h"
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

TEST_CASE("cut_flow_preset_generates_plugin_spec") {
    PluginArgs vars{{"plot_configs",
                     {{"selection_rule", "SEL"},
                      {"region", "R"},
                      {"signal_group", "inclusive_strange_channels"},
                      {"channel_column", "channel_definitions"},
                      {"initial_label", "Start"},
                      {"plot_name", "my_plot"}}}};
    auto pr = PresetRegistry::instance().find("CUT_FLOW_PLOT");
    REQUIRE(pr != nullptr);
    auto list = pr->make(vars);
    REQUIRE(list.size() == 1);
    auto spec = list.front();
    REQUIRE(spec.id == "CutFlowPlotPlugin");
    REQUIRE(spec.args.plot_configs.contains("plots"));
    auto plots = spec.args.plot_configs.at("plots");
    REQUIRE(plots.is_array());
    REQUIRE(plots.size() == 1);
    auto cf = plots.at(0);
    REQUIRE(cf.at("selection_rule") == "SEL");
    REQUIRE(cf.at("region") == "R");
    REQUIRE(cf.at("signal_group") == "inclusive_strange_channels");
    REQUIRE(cf.at("channel_column") == "channel_definitions");
    REQUIRE(cf.at("initial_label") == "Start");
    REQUIRE(cf.at("plot_name") == "my_plot");
    REQUIRE(cf.at("output_directory") == "./plots/cut_flow");
    REQUIRE(cf.at("log_y") == false);
    REQUIRE(cf.at("clauses").is_array());
    REQUIRE(cf.at("clauses").empty());
}

