#include <catch2/catch_test_macros.hpp>
#include <cmath>
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
TEST_CASE("stacked histogram uniform binning") {
    std::vector<double> edges{0.0, 1.0, 2.0, 3.0, 4.0};
    BinningDefinition binning(edges, "x", "x", {});
    std::vector<double> counts{1.0, 2.0, 3.0, 4.0};
    Eigen::VectorXd sh_vec = Eigen::VectorXd::Zero(counts.size());
    Eigen::MatrixXd shifts = sh_vec;
    BinnedHistogram hist(binning, counts, shifts);
    VariableResult result;
    result.binning_ = binning;
    result.total_mc_hist_ = hist;
    result.strat_hists_.emplace(ChannelKey{std::string{"10"}}, hist);
    RegionAnalysis region(RegionKey{std::string{"reg"}}, "reg");
    StackedHistogramPlot plot("test_plot", result, region, "inclusive_strange_channels", "test_plots", true, {}, true, false, "Events", 2, 0.0, 4.0);
    plot.drawAndSave("root");
    TFile file("test_plots/test_plot.root");
    auto canvas = dynamic_cast<TCanvas *>(file.Get("test_plot"));
    CHECK(canvas);
    auto stack = dynamic_cast<THStack *>(canvas->GetPrimitive("mc_stack"));
    CHECK(stack);
    TH1 *frame = stack->GetHistogram();
    CHECK(frame);
    auto axis = frame->GetXaxis();
    CHECK(axis->GetNbins() == 2);
    CHECK(std::abs(axis->GetXmin() + 0.2) < 1e-6);
    CHECK(std::abs(axis->GetXmax() - 4.2) < 1e-6);
}
