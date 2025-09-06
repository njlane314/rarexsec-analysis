#include "SystematicBreakdownPlot.h"

#include <cmath>

#include "TCanvas.h"
#include "TColor.h"

namespace analysis {

SystematicBreakdownPlot::SystematicBreakdownPlot(
    std::string plot_name,
    const VariableResult &var_result,
    bool normalise,
    std::string output_directory)
    : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)),
      variable_result_(var_result),
      normalise_(normalise) {}

std::vector<double> SystematicBreakdownPlot::calculateTotals() const {
    const int nbins = variable_result_.binning_.getBinNumber();
    std::vector<double> bin_totals(nbins, 0.0);
    for (const auto &[key, cov] : variable_result_.covariance_matrices_) {
        const int n = cov.GetNrows();
        for (int i = 0; i < nbins && i < n; ++i) {
            double val = cov(i, i);
            if (std::isfinite(val)) {
                bin_totals[i] += val;
            }
        }
    }

    return bin_totals;
}

void SystematicBreakdownPlot::fillHistograms(
    const std::vector<double> &totals) {
    const auto &edges = variable_result_.binning_.getEdges();
    const int nbins = variable_result_.binning_.getBinNumber();

    stack_ = std::make_unique<THStack>("syst_stack", "");
    legend_ = std::make_unique<TLegend>(0.65, 0.7, 0.9, 0.9);
    legend_->SetBorderSize(0);
    legend_->SetFillStyle(0);
    legend_->SetTextFont(42);

    histograms_.clear();

    int colour = kRed + 1;
    for (const auto &[key, cov] : variable_result_.covariance_matrices_) {
        auto hist =
            std::make_unique<TH1D>(key.str().c_str(), "", nbins, edges.data());
        const int n = cov.GetNrows();
        for (int i = 0; i < nbins && i < n; ++i) {
            double val = cov(i, i);
            if (std::isfinite(val)) {
                if (normalise_ && totals[i] > 0.0) {
                    val /= totals[i];
                }
                hist->SetBinContent(i + 1, val);
            } else {
                hist->SetBinContent(i + 1, 0.0);
            }
        }
        hist->SetFillColor(colour);
        hist->SetLineColor(kBlack);
        stack_->Add(hist.get());
        legend_->AddEntry(hist.get(), key.str().c_str(), "f");
        histograms_.push_back(std::move(hist));
        ++colour;
    }
}

void SystematicBreakdownPlot::renderStackLegend() {
    stack_->Draw("hist");
    stack_->GetXaxis()->SetTitle(
        variable_result_.binning_.getTexLabel().c_str());
    stack_->GetYaxis()->SetTitle(
        normalise_ ? "Fractional Contribution" : "Variance");

    legend_->Draw();
}

void SystematicBreakdownPlot::draw(TCanvas &canvas) {
    canvas.cd();

    auto totals = calculateTotals();
    fillHistograms(totals);
    renderStackLegend();
}

}

