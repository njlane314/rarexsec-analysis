#include "PresetRegistry.h"
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

TEST_CASE("topological_score_preset_generates_variable_spec") {
  auto pr = PresetRegistry::instance().find("TEST_TOPOLOGICAL_SCORE");
  REQUIRE(pr != nullptr);
  auto list = pr->make({});
  REQUIRE(list.size() == 1);
  auto spec = list.front();
  REQUIRE(spec.id == "VariablesPlugin");
  REQUIRE(spec.args.analysis_configs.contains("variables"));
  auto vars = spec.args.analysis_configs.at("variables");
  REQUIRE(vars.is_array());
  REQUIRE(vars.size() == 1);
  auto var = vars.at(0);
  REQUIRE(var.at("name") == "topological_score");
  auto bins = var.at("bins");
  REQUIRE(bins.at("n") == 10);
  REQUIRE(bins.at("min") == 0.0);
  REQUIRE(bins.at("max") == 1.0);
}
