#include "BinningDefinition.h"
#include "DetectorSystematicStrategy.h"
#include "SystematicsProcessor.h"
#include "UniverseSystematicStrategy.h"
#include "VariableResult.h"
#include "WeightSystematicStrategy.h"
#include <Eigen/Eigenvalues>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <string>
#include <vector>

using namespace analysis;

namespace {
BinningDefinition makeBinning() {
  std::vector<double> e{0.0, 1.0, 2.0};
  return BinningDefinition(e, "x", "x", {});
}
VariableResult makeResult(const BinningDefinition &b) {
  VariableResult r;
  r.binning_ = b;
  std::vector<double> c{1.0, 1.0};
  Eigen::MatrixXd m = Eigen::MatrixXd::Identity(2, 2);
  r.total_mc_hist_ = BinnedHistogram(b, c, m);
  return r;
}
// build C = (1/N) sum v v^T from supplied variation vectors
Eigen::MatrixXd covMatrix(const std::vector<Eigen::VectorXd> &v) {
  Eigen::MatrixXd c = Eigen::MatrixXd::Zero(v[0].size(), v[0].size());
  for (const auto &u : v)
    c += u * u.transpose();
  return c / v.size();
}
// verify positive semidefinite: all lambda_i >= 0 within tolerance
bool psd(const Eigen::MatrixXd &m) {
  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es(m);
  return (es.eigenvalues().array() >= -1e-12).all();
}
Eigen::MatrixXd toEigen(const TMatrixDSym &m) {
  Eigen::MatrixXd out(m.GetNrows(), m.GetNcols());
  for (int i = 0; i < m.GetNrows(); ++i)
    for (int j = 0; j < m.GetNcols(); ++j)
      out(i, j) = m(i, j);
  return out;
}
} // namespace

// Processor: check C_total = sum C_s + I and propagated errors sqrt(diag
// C_total)
TEST_CASE("systematics processor covariance") {
  auto b = makeBinning();
  std::vector<double> x{0.5, 1.5};
  std::vector<double> up{1.2, 0.8};
  std::vector<double> dn{0.8, 1.2};
  std::vector<ROOT::RVec<unsigned short>> u{ROOT::RVec<unsigned short>{2, 0},
                                            ROOT::RVec<unsigned short>{0, 2}};
  ROOT::RDataFrame df(x.size());
  ROOT::RDF::RNode rnode =
      df.Define("x", [&x](ULong64_t i) { return x[i]; }, {"rdfentry_"})
          .Define("knob_up", [&up](ULong64_t i) { return up[i]; },
                  {"rdfentry_"})
          .Define("knob_dn", [&dn](ULong64_t i) { return dn[i]; },
                  {"rdfentry_"})
          .Define("uni_weights", [&u](ULong64_t i) { return u[i]; },
                  {"rdfentry_"});
  KnobDef k{"knob", "knob_up", "knob_dn"};
  UniverseDef un{"uni", "uni_weights", 2};
  SystematicsProcessor p({k}, {un});
  SampleKey sk(std::string{"sample"});
  p.bookSystematics(sk, rnode, b, b.toTH1DModel());
  auto r = makeResult(b);
  r.raw_detvar_hists_[sk][SampleVariation::kCV] =
      BinnedHistogram(b, {1.0, 1.0}, Eigen::MatrixXd::Zero(2, 1));
  r.raw_detvar_hists_[sk][SampleVariation::kSCE] =
      BinnedHistogram(b, {1.1, 0.9}, Eigen::MatrixXd::Zero(2, 1));
  p.processSystematics(r);
  // C_w from delta w = plus or minus 0.2: average of [delta w,-delta w]^T[delta
  // w,-delta w]
  Eigen::MatrixXd wexp =
      covMatrix({Eigen::Vector2d(0.2, -0.2), Eigen::Vector2d(-0.2, 0.2)});
  // C_u from universes giving plus or minus 1 deviations in opposite bins
  Eigen::MatrixXd uexp =
      covMatrix({Eigen::Vector2d(1, -1), Eigen::Vector2d(-1, 1)});
  // C_d from detector shift Delta=[0.1,-0.1]
  Eigen::MatrixXd dexp = covMatrix({Eigen::Vector2d(0.1, -0.1)});
  CHECK(
      (toEigen(r.covariance_matrices_.at(SystematicKey(std::string{"knob"}))) -
       wexp)
          .norm() < 1e-6);
  CHECK((toEigen(r.covariance_matrices_.at(SystematicKey(std::string{"uni"}))) -
         uexp)
            .norm() < 1e-6);
  CHECK((toEigen(r.covariance_matrices_.at(
             SystematicKey(std::string{"detector_variation"}))) -
         dexp)
            .norm() < 1e-6);
  CHECK(psd(
      toEigen(r.covariance_matrices_.at(SystematicKey(std::string{"knob"})))));
  CHECK(psd(
      toEigen(r.covariance_matrices_.at(SystematicKey(std::string{"uni"})))));
  CHECK(psd(toEigen(r.covariance_matrices_.at(
      SystematicKey(std::string{"detector_variation"})))));
  Eigen::MatrixXd totalexp =
      wexp + uexp + dexp + Eigen::MatrixXd::Identity(2, 2);
  CHECK((toEigen(r.total_covariance_) - totalexp).norm() < 1e-6);
  CHECK(psd(toEigen(r.total_covariance_)));
  Eigen::Vector2d err{std::sqrt(totalexp(0, 0)), std::sqrt(totalexp(1, 1))};
  CHECK((Eigen::Vector2d(r.nominal_with_band_.getBinError(0),
                         r.nominal_with_band_.getBinError(1)) -
         err)
            .norm() < 1e-6);
  CHECK(r.universe_projected_hists_.empty());
}

// Universes: deviations plus or minus 1 yield off-diagonal covariance
TEST_CASE("universe systematic strategy covariance") {
  auto b = makeBinning();
  std::vector<double> x{0.5, 1.5};
  std::vector<ROOT::RVec<unsigned short>> u{ROOT::RVec<unsigned short>{2, 0},
                                            ROOT::RVec<unsigned short>{0, 2}};
  ROOT::RDataFrame df(x.size());
  ROOT::RDF::RNode rnode =
      df.Define("x", [&x](ULong64_t i) { return x[i]; }, {"rdfentry_"})
          .Define("uni_weights", [&u](ULong64_t i) { return u[i]; },
                  {"rdfentry_"});
  UniverseSystematicStrategy s(UniverseDef{"uni", "uni_weights", 2});
  SystematicFutures f;
  SampleKey sk(std::string{"s"});
  s.bookVariations(sk, rnode, b, b.toTH1DModel(), f);
  auto r = makeResult(b);
  auto cov = s.computeCovariance(r, f);
  // C = 1/2([1,-1]^T[1,-1] + [-1,1]^T[-1,1])
  Eigen::MatrixXd exp =
      covMatrix({Eigen::Vector2d(1, -1), Eigen::Vector2d(-1, 1)});
  CHECK((toEigen(cov) - exp).norm() < 1e-6);
  CHECK(psd(toEigen(cov)));
}

// Ensure that float-based weight vectors are handled correctly.
TEST_CASE("universe systematic strategy covariance (float weights)") {
  auto b = makeBinning();
  std::vector<double> x{0.5, 1.5};
  std::vector<ROOT::RVec<float>> u{ROOT::RVec<float>{1.1F, 0.9F},
                                   ROOT::RVec<float>{0.9F, 1.1F}};
  ROOT::RDataFrame df(x.size());
  ROOT::RDF::RNode rnode =
      df.Define("x", [&x](ULong64_t i) { return x[i]; }, {"rdfentry_"})
          .Define("uni_weights", [&u](ULong64_t i) { return u[i]; },
                  {"rdfentry_"});
  UniverseSystematicStrategy s(UniverseDef{"uni", "uni_weights", 2});
  SystematicFutures f;
  SampleKey sk(std::string{"s"});
  s.bookVariations(sk, rnode, b, b.toTH1DModel(), f);
  auto r = makeResult(b);
  auto cov = s.computeCovariance(r, f);
  // Expected covariance from variations of ±0.1 around the nominal weight
  Eigen::MatrixXd exp =
      covMatrix({Eigen::Vector2d(0.1, -0.1), Eigen::Vector2d(-0.1, 0.1)});
  CHECK((toEigen(cov) - exp).norm() < 1e-6);
  CHECK(psd(toEigen(cov)));
}

// Weight shifts: symmetric plus or minus 0.2 produce diagonal covariance
TEST_CASE("weight systematic strategy covariance") {
  auto b = makeBinning();
  std::vector<double> x{0.5, 1.5};
  std::vector<double> up{1.2, 0.8};
  std::vector<double> dn{0.8, 1.2};
  ROOT::RDataFrame df(x.size());
  ROOT::RDF::RNode rnode =
      df.Define("x", [&x](ULong64_t i) { return x[i]; }, {"rdfentry_"})
          .Define("knob_up", [&up](ULong64_t i) { return up[i]; },
                  {"rdfentry_"})
          .Define("knob_dn", [&dn](ULong64_t i) { return dn[i]; },
                  {"rdfentry_"});
  WeightSystematicStrategy s(KnobDef{"k", "knob_up", "knob_dn"});
  SystematicFutures f;
  SampleKey sk(std::string{"s"});
  s.bookVariations(sk, rnode, b, b.toTH1DModel(), f);
  auto r = makeResult(b);
  auto cov = s.computeCovariance(r, f);
  // Average vv^T for v=[0.2,-0.2] gives 0.04 on the diagonal
  Eigen::MatrixXd exp =
      covMatrix({Eigen::Vector2d(0.2, -0.2), Eigen::Vector2d(-0.2, 0.2)});
  CHECK((toEigen(cov) - exp).norm() < 1e-6);
  CHECK(psd(toEigen(cov)));
}

// Detector variation: single shift Delta=[0.1,-0.1] => C=Delta Delta^T
TEST_CASE("detector systematic strategy covariance") {
  auto b = makeBinning();
  auto r = makeResult(b);
  SampleKey sk(std::string{"s"});
  r.raw_detvar_hists_[sk][SampleVariation::kCV] =
      BinnedHistogram(b, {1.0, 1.0}, Eigen::MatrixXd::Zero(2, 1));
  r.raw_detvar_hists_[sk][SampleVariation::kSCE] =
      BinnedHistogram(b, {1.1, 0.9}, Eigen::MatrixXd::Zero(2, 1));
  DetectorSystematicStrategy s;
  SystematicFutures f;
  auto cov = s.computeCovariance(r, f);
  Eigen::MatrixXd exp = covMatrix({Eigen::Vector2d(0.1, -0.1)});
  CHECK((toEigen(cov) - exp).norm() < 1e-6);
  CHECK(psd(toEigen(cov)));
}
