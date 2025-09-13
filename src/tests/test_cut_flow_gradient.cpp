#include <rarexsec/core/CutFlowGradient.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace analysis;

namespace {
RegionAnalysis::StageCount make_stage(double sig, double bkg) {
    RegionAnalysis::StageCount sc;
    sc.total = sig + bkg;
    sc.schemes["chan"][1] = {sig, sig}; // signal key 1
    sc.schemes["chan"][2] = {bkg, bkg}; // background key 2
    return sc;
}
}

TEST_CASE("compute finite difference gradients for cut flow") {
    std::vector<RegionAnalysis::StageCount> plus{
        make_stage(100.0, 200.0), // initial
        make_stage(61.0, 30.0),   // after cut1
        make_stage(31.0, 15.0)    // after cut2
    };

    std::vector<RegionAnalysis::StageCount> minus{
        make_stage(100.0, 200.0),
        make_stage(59.0, 50.0),
        make_stage(29.0, 25.0)
    };

    auto grad = computeCutFlowGradient(plus, minus, "chan", 1, {2});

    REQUIRE(grad.signal.size() == 3);
    REQUIRE(grad.backgrounds.at(2).size() == 3);

    // Stage 1 gradients
    CHECK(grad.signal[1] == Catch::Approx((0.61 - 0.59) / 2.0));
    CHECK(grad.backgrounds.at(2)[1] == Catch::Approx((0.15 - 0.25) / 2.0));

    // Stage 2 gradients
    CHECK(grad.signal[2] == Catch::Approx((0.31 - 0.29) / 2.0));
    CHECK(grad.backgrounds.at(2)[2] == Catch::Approx((0.075 - 0.125) / 2.0));
}
