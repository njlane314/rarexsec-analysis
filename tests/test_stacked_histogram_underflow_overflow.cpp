#include <catch2/catch_test_macros.hpp>
#include <vector>
#include "Eigen/Dense"
#include "TCanvas.h"
#include "TFile.h"
#include "TH1.h"
#include "THStack.h"
#include "AnalysisTypes.h"
#include "BinningDefinition.h"
#include "RegionAnalysis.h"
#include "StackedHistogramPlot.h"
using namespace analysis;
TEST_CASE("stacked histogram underflow overflow") {
    std::vector<double> edges{-1.0, 0.0, 1.0, 2.0, 3.0, 4.0};
    BinningDefinition binning(edges, "x", "x", {});
    std::vector<double> counts{5.0, 1.0, 2.0, 3.0, 6.0};
    Eigen::VectorXd sh_vec = Eigen::VectorXd::Zero(counts.size());
    Eigen::MatrixXd shifts = sh_vec;
    BinnedHistogram hist(binning, counts, shifts);
    VariableResult result;
    result.binning_ = binning;
    result.total_mc_hist_ = hist;
    result.strat_hists_.emplace(ChannelKey{std::string{"10"}}, hist);
    RegionAnalysis region(RegionKey{std::string{"reg"}}, "reg");
    StackedHistogramPlot plot("under_over_test", result, region, "inclusive_strange_channels", "test_plots", true, {}, true, false, "Events");
    plot.drawAndSave("root");
    TFile file("test_plots/under_over_test.root");
    auto canvas = dynamic_cast<TCanvas *>(file.Get("under_over_test"));
    CHECK(canvas);
    auto stack = dynamic_cast<THStack *>(canvas->GetPrimitive("mc_stack"));
    CHECK(stack);
    TH1 *frame = stack->GetHistogram();
    CHECK(frame);
    CHECK(frame->GetNbinsX() == 5);
    CHECK(std::abs(frame->GetBinContent(1) - 5.0) < 1e-6);
    CHECK(std::abs(frame->GetBinContent(5) - 6.0) < 1e-6);
    auto xaxis = frame->GetXaxis();
    CHECK(std::string(xaxis->GetBinLabel(1)) == "<-1");
    CHECK(std::string(xaxis->GetBinLabel(xaxis->GetNbins())) == ">4");
}
