#ifndef SYSTEMATICBREAKDOWNPLOT_H
#define SYSTEMATICBREAKDOWNPLOT_H

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TColor.h"
#include "TH1D.h"
#include "THStack.h"
#include "TLegend.h"

#include <rarexsec/plot/IHistogramPlot.h>
#include <rarexsec/app/VariableResult.h>

namespace analysis {

class SystematicBreakdownPlot : public IHistogramPlot {
  public:
    SystematicBreakdownPlot(std::string plot_name,
                            const VariableResult &var_result,
                            bool normalise = false,
                            std::string output_directory = "plots")
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
          variable_result_(var_result), normalise_(normalise), stack_(nullptr),
          legend_(nullptr) {}

    ~SystematicBreakdownPlot() override {
        delete stack_;
        delete legend_;
        for (auto *h : histograms_) {
            delete h;
        }
    }

  protected:
    void draw(TCanvas &canvas) override {
        canvas.cd();

        const auto &edges = variable_result_.binning_.getEdges();
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

        stack_ = new THStack("syst_stack", "");
        const double x1 = 0.65;
        const double y1 = 0.7;
        const double x2 = 0.9;
        const double y2 = 0.9;
        const int border = 0;
        const int fill = 0;
        const int font_style = 42;
        const int colour_offset = 1;
        const int bin_offset = 1;
        const double zero = 0.0;

        legend_ = new TLegend(x1, y1, x2, y2);
        legend_->SetBorderSize(border);
        legend_->SetFillStyle(fill);
        legend_->SetTextFont(font_style);

        int colour = kRed + colour_offset;
        for (const auto &[key, cov] : variable_result_.covariance_matrices_) {
            TH1D *hist = new TH1D(key.str().c_str(), "", nbins, edges.data());
            const int n = cov.GetNrows();
            for (int i = 0; i < nbins && i < n; ++i) {
                double val = cov(i, i);
                if (std::isfinite(val)) {
                    if (normalise_ && bin_totals[i] > zero) {
                        val /= bin_totals[i];
                    }
                    hist->SetBinContent(i + bin_offset, val);
                } else {
                    hist->SetBinContent(i + bin_offset, zero);
                }
            }
            hist->SetFillColor(colour);
            hist->SetLineColor(kBlack);
            stack_->Add(hist);
            legend_->AddEntry(hist, key.str().c_str(), "f");
            histograms_.push_back(hist);
            ++colour;
        }

        stack_->Draw("hist");
        stack_->GetXaxis()->SetTitle(
            variable_result_.binning_.getTexLabel().c_str());
        stack_->GetYaxis()->SetTitle(normalise_ ? "Fractional Contribution"
                                                : "Variance");
        legend_->Draw();
    }

  private:
    const VariableResult &variable_result_;
    bool normalise_;
    THStack *stack_;
    std::vector<TH1D *> histograms_;
    TLegend *legend_;
};

}

#endif
