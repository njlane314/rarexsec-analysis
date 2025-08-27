#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include "HistogramUncertainty.h"
using namespace analysis;
TEST_CASE("histogram uncertainty operations") {
    BinningDefinition bn({0.0, 1.0, 2.0}, "", "", {}, "");
    std::vector<double> c1{1.0, 2.0};
    Eigen::MatrixXd s1 = Eigen::Vector2d(0.1, 0.2);
    HistogramUncertainty h1(bn, c1, s1);
    std::vector<double> c2{3.0, 4.0};
    Eigen::MatrixXd s2 = Eigen::Vector2d(0.3, 0.4);
    HistogramUncertainty h2(bn, c2, s2);
    auto h_sum = h1 + h2;
    CHECK(h_sum.count(0) == 4.0);
    CHECK(h_sum.count(1) == 6.0);
    CHECK(std::abs(h_sum.err(0) - std::sqrt(0.1 * 0.1 + 0.3 * 0.3)) < 1e-12);
    CHECK(std::abs(h_sum.err(1) - std::sqrt(0.2 * 0.2 + 0.4 * 0.4)) < 1e-12);
    auto corr = h_sum.corrMat();
    CHECK(std::abs(corr(0, 1)) < 1e-12);
    CHECK(std::abs(corr(0, 0) - 1.0) < 1e-12);
    auto h_mul = h1 * h2;
    CHECK(h_mul.count(0) == 3.0);
    double expected_err0 = std::abs(3.0) * std::sqrt(std::pow(0.1 / 1.0, 2) + std::pow(0.3 / 3.0, 2));
    CHECK(std::abs(h_mul.err(0) - expected_err0) < 1e-12);
    auto h_div = h1 / h2;
    CHECK(std::abs(h_div.count(0) - (1.0 / 3.0)) < 1e-12);
    double expected_div_err0 = std::abs(1.0 / 3.0) * std::sqrt(std::pow(0.1 / 1.0, 2) + std::pow(0.3 / 3.0, 2));
    CHECK(std::abs(h_div.err(0) - expected_div_err0) < 1e-12);
    HistogramUncertainty h3(bn, {1.0, 1.0}, Eigen::Vector2d(0.1, 0.2));
    TMatrixDSym cov(2);
    cov(0, 0) = 0.01;
    cov(1, 1) = 0.04;
    cov(0, 1) = cov(1, 0) = 0.02;
    h3.addCovariance(cov);
    auto corr2 = h3.corrMat();
    double expected_corr = 0.02 / std::sqrt((0.01 + 0.01) * (0.04 + 0.04));
    CHECK(std::abs(corr2(0, 1) - expected_corr) < 1e-12);
    BinningDefinition empty_bn;
    CHECK(empty_bn.getBinNumber() == 0);
    HistogramUncertainty empty_hist;
    auto h_sum2 = empty_hist + h1;
    CHECK(h_sum2.size() == h1.size());
    CHECK(h_sum2.count(0) == h1.count(0));
}
