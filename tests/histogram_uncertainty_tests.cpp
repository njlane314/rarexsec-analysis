#include <cassert>
#include <cmath>

#include "HistogramUncertainty.h"

using namespace analysis;

int main() {
    BinningDefinition bn({0.0, 1.0, 2.0}, "", "", {}, "");

    std::vector<double> c1{1.0, 2.0};
    Eigen::MatrixXd s1 = Eigen::Vector2d(0.1, 0.2);
    HistogramUncertainty h1(bn, c1, s1);

    std::vector<double> c2{3.0, 4.0};
    Eigen::MatrixXd s2 = Eigen::Vector2d(0.3, 0.4);
    HistogramUncertainty h2(bn, c2, s2);

    auto h_sum = h1 + h2;
    assert(h_sum.count(0) == 4.0);
    assert(h_sum.count(1) == 6.0);
    assert(std::abs(h_sum.err(0) - std::sqrt(0.1*0.1 + 0.3*0.3)) < 1e-12);
    assert(std::abs(h_sum.err(1) - std::sqrt(0.2*0.2 + 0.4*0.4)) < 1e-12);
    auto corr = h_sum.corrMat();
    assert(std::abs(corr(0,1)) < 1e-12);
    assert(std::abs(corr(0,0) - 1.0) < 1e-12);

    auto h_mul = h1 * h2;
    assert(h_mul.count(0) == 3.0);
    double expected_err0 = std::abs(3.0) * std::sqrt(std::pow(0.1/1.0,2) + std::pow(0.3/3.0,2));
    assert(std::abs(h_mul.err(0) - expected_err0) < 1e-12);

    auto h_div = h1 / h2;
    assert(std::abs(h_div.count(0) - (1.0/3.0)) < 1e-12);
    double expected_div_err0 = std::abs(1.0/3.0) * std::sqrt(std::pow(0.1/1.0,2) + std::pow(0.3/3.0,2));
    assert(std::abs(h_div.err(0) - expected_div_err0) < 1e-12);

    HistogramUncertainty h3(bn, {1.0,1.0}, Eigen::Vector2d(0.1,0.2));
    TMatrixDSym cov(2);
    cov(0,0) = 0.01;
    cov(1,1) = 0.04;
    cov(0,1) = cov(1,0) = 0.02;
    h3.addCovariance(cov);
    auto corr2 = h3.corrMat();
    double expected_corr = 0.02 / std::sqrt((0.01+0.01)*(0.04+0.04));
    assert(std::abs(corr2(0,1) - expected_corr) < 1e-12);

    return 0;
}
