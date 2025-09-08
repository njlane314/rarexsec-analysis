#include "PresetRegistry.h"
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

TEST_CASE("stacked_log_preset_generates_plugin_spec") {
    auto pr = PresetRegistry::instance().find("STACKED_PLOTS_LOG");
    REQUIRE(pr != nullptr);
    auto list = pr->make({});
    REQUIRE(list.size() == 1);
    auto spec = list.front();
    REQUIRE(spec.id == "StackedHistogramPlugin");
    REQUIRE(spec.args.plot_configs.contains("plots"));
    auto plots = spec.args.plot_configs.at("plots");
    REQUIRE(plots.is_array());
    REQUIRE(plots.size() == 1);
    auto plot = plots.at(0);
    REQUIRE(plot.at("category_column") == "channel_definitions");
    REQUIRE(plot.at("log_y") == true);
}
