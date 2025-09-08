#include "PresetRegistry.h"
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

TEST_CASE("event_display_preset_generates_plugin_spec") {
    PluginArgs vars{{"plot_configs", {{"sample", "s"}, {"region", "R"}}}};
    auto pr = PresetRegistry::instance().find("EVENT_DISPLAY");
    REQUIRE(pr != nullptr);
    auto list = pr->make(vars);
    REQUIRE(list.size() == 1);
    auto spec = list.front();
    REQUIRE(spec.id == "EventDisplayPlugin");
    REQUIRE(spec.args.plot_configs.contains("event_displays"));
    auto eds = spec.args.plot_configs.at("event_displays");
    REQUIRE(eds.is_array());
    REQUIRE(eds.size() == 1);
    auto ed = eds.at(0);
    REQUIRE(ed.at("sample") == "s");
    REQUIRE(ed.at("region") == "R");
    REQUIRE(ed.at("n_events") == 1);
    REQUIRE(ed.at("image_size") == 800);
    REQUIRE(ed.at("output_directory") == "./plots/event_displays");
}
