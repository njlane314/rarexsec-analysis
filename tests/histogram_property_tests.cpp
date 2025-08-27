#include <catch2/catch_test_macros.hpp>
#include <random>

#include "HistogramUncertainty.h"

using namespace analysis;

TEST_CASE("HistogramUncertainty addition is commutative", "[property]") {
    BinningDefinition bn({0.0, 1.0, 2.0}, "", "", {}, "");
    std::mt19937 gen(std::random_device{}());
    std::uniform_real_distribution<double> count_dist(0.0, 10.0);
    std::uniform_real_distribution<double> err_dist(0.0, 1.0);

    for (int i = 0; i < 100; ++i) {
        std::vector<double> c1{count_dist(gen), count_dist(gen)};
        Eigen::Vector2d e1(err_dist(gen), err_dist(gen));
        HistogramUncertainty h1(bn, c1, e1);

        std::vector<double> c2{count_dist(gen), count_dist(gen)};
        Eigen::Vector2d e2(err_dist(gen), err_dist(gen));
        HistogramUncertainty h2(bn, c2, e2);

        auto sum1 = h1 + h2;
        auto sum2 = h2 + h1;

        REQUIRE(sum1.count(0) == Approx(sum2.count(0)));
        REQUIRE(sum1.count(1) == Approx(sum2.count(1)));
        REQUIRE(sum1.err(0) == Approx(sum2.err(0)));
        REQUIRE(sum1.err(1) == Approx(sum2.err(1)));
    }
}
