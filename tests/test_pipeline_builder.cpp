#include "PipelineBuilder.h"
#include "PresetRegistry.h"
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

TEST_CASE("pipeline_builder_requires_region_and_variable") {
    AnalysisPluginHost a_host;
    PlotPluginHost p_host;
    PipelineBuilder builder(a_host, p_host);

    builder.region("EMPTY");
    REQUIRE_THROWS(builder.analysisSpecs());

    builder.variable("TRUE_NEUTRINO_VERTEX");
    REQUIRE_NOTHROW(builder.analysisSpecs());
}
