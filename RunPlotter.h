#ifndef RUN_PLOTTER_H
#define RUN_PLOTTER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <cmath>
#include <cstring>

#include "TStyle.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TString.h"
#include "THStack.h"
#include "TLegend.h"
#include "TColor.h"
#include "TROOT.h"

#include "RunHistGenerator.h"
#include "Histogram.h"

namespace AnalysisFramework {

class RunPlotter {
public:
    RunPlotter(const RunHistGenerator& run_hist_generator)
        : run_hist_generator_(run_hist_generator) {
            this->InitialiseCommonStyle();
        }

    void Plot(
        TCanvas* canvas,
        const std::string& category_column = "event_category",
        double data_pot = 1.0
    ) const {
        if (!canvas) {
            std::cerr << "Error: RunPlotter::Plot called with a null canvas." << std::endl;
            return;
        }

        auto mc_hists_map = run_hist_generator_.GetMonteCarloHists(category_column.c_str());
        std::vector<AnalysisFramework::Histogram> mc_hists_vec;
        for (const auto& pair : mc_hists_map) {
            mc_hists_vec.push_back(pair.second);
        }

        if (mc_hists_vec.empty()) {
            std::cerr << "Warning: No Monte Carlo histograms found to plot for category: " << category_column << std::endl;
            return;
        }

        this->PlotStackedHistogram(canvas, mc_hists_vec);
    }

private:
    const RunHistGenerator& run_hist_generator_;
    mutable std::unique_ptr<THStack> current_stack;
    mutable std::unique_ptr<TH1> current_total_hist; // Added to manage total_hist lifetime

    void InitialiseCommonStyle() {
        const int font_style = 132;
        TStyle* style = new TStyle("ConsistentStyle", "Consistent Style for Analysis Plots");

        style->SetTitleFont(font_style, "X");
        style->SetTitleFont(font_style, "Y");
        style->SetTitleFont(font_style, "Z");
        style->SetTitleSize(0.04, "X");
        style->SetTitleSize(0.04, "Y");
        style->SetTitleSize(0.04, "Z");

        style->SetLabelFont(font_style, "X");
        style->SetLabelFont(font_style, "Y");
        style->SetLabelFont(font_style, "Z");
        style->SetLabelSize(0.035, "X");
        style->SetLabelSize(0.035, "Y");
        style->SetLabelSize(0.035, "Z");

        style->SetTitleOffset(1.1, "X");
        style->SetTitleOffset(1.1, "Y");
        style->SetTitleOffset(1.1, "Z");

        style->SetOptStat(0);

        style->SetPadTickX(0);
        style->SetPadTickY(0);
        style->SetPadLeftMargin(0.15);
        style->SetPadRightMargin(0.05);
        style->SetPadTopMargin(0.07);
        style->SetPadBottomMargin(0.12);

        style->SetMarkerSize(1.0);

        style->SetCanvasColor(0);
        style->SetPadColor(0);
        style->SetFrameFillColor(0);

        gROOT->SetStyle("ConsistentStyle");
        gROOT->ForceStyle();
    }

    TH1* CreateTotalUncertaintyHistogram(const std::vector<AnalysisFramework::Histogram>& hist_vec, THStack* stack) const {
        if (!stack || !stack->GetStack() || !stack->GetStack()->Last()) {
            return nullptr;
        }

        TH1* total_hist = static_cast<TH1*>(stack->GetStack()->Last()->Clone("total_mc_uncertainty"));
        total_hist->SetDirectory(nullptr);

        for (int i = 1; i <= total_hist->GetNbinsX(); ++i) {
            double total_error_squared = 0.0;
            for (const auto& an_hist_wrapper : hist_vec) {
                TH1* h_comp = an_hist_wrapper.getRootHistCopy();
                if (h_comp) {
                    double err = h_comp->GetBinError(i);
                    total_error_squared += err * err;
                    delete h_comp;
                }
            }
            double total_error = std::sqrt(total_error_squared);
            total_hist->SetBinError(i, total_error);
        }

        return total_hist;
    }

    void SetErrorHistogramStyle(TH1* total_hist) const {
        total_hist->SetMarkerSize(0);
        total_hist->SetFillColor(kBlack);
        total_hist->SetLineColor(kBlack);
        total_hist->SetLineWidth(2);
        total_hist->SetFillStyle(3004);
    }

    void PlotStackedHistogram(
        TCanvas* canvas,
        const std::vector<AnalysisFramework::Histogram>& hist_vec
    ) const {
        if (!canvas) {
            std::cerr << "Error: Canvas is null in PlotStackedHistogram." << std::endl;
            return;
        }
        canvas->cd();

        current_stack = std::make_unique<THStack>("mc_stack", "");

        std::string common_x_axis_title = "X-axis Title (Default)";
        std::string common_y_axis_title = "Events";
        bool first_hist = true;

        for (const auto& an_hist_wrapper : hist_vec) {
            TH1* root_hist = an_hist_wrapper.getRootHistCopy();
            if (!root_hist) {
                std::cerr << "Warning: Encountered a null histogram." << std::endl;
                continue;
            }

            bool has_errors = false;
            for (int i = 1; i <= root_hist->GetNbinsX(); ++i) {
                if (root_hist->GetBinError(i) > 0) {
                    has_errors = true;
                    break;
                }
            }
            if (!has_errors) {
                std::cout << "Note: Setting errors for " << root_hist->GetName() << " to sqrt(content)." << std::endl;
                for (int i = 1; i <= root_hist->GetNbinsX(); ++i) {
                    double content = root_hist->GetBinContent(i);
                    root_hist->SetBinError(i, content > 0 ? std::sqrt(content) : 0);
                }
            }

            root_hist->SetLineColor(kBlack);
            root_hist->SetLineWidth(1);
            current_stack->Add(root_hist);
            if (first_hist) {
                if (root_hist->GetXaxis()->GetTitle() && strlen(root_hist->GetXaxis()->GetTitle()) > 0) {
                    common_x_axis_title = root_hist->GetXaxis()->GetTitle();
                }
                if (root_hist->GetYaxis()->GetTitle() && strlen(root_hist->GetYaxis()->GetTitle()) > 0) {
                    common_y_axis_title = root_hist->GetYaxis()->GetTitle();
                }
                first_hist = false;
            }
        }

        if (!current_stack->GetHists() || current_stack->GetHists()->GetSize() == 0) {
            std::cerr << "Warning: THStack is empty. Nothing to draw." << std::endl;
            return;
        }

        current_stack->Draw("HIST");

        if (current_stack->GetHistogram()) {
            current_stack->GetHistogram()->GetXaxis()->SetTitle(common_x_axis_title.c_str());
            current_stack->GetHistogram()->GetYaxis()->SetTitle(common_y_axis_title.c_str());
            current_stack->GetHistogram()->GetXaxis()->CenterTitle();
            current_stack->GetHistogram()->GetYaxis()->CenterTitle();
        }

        TH1* total_hist = CreateTotalUncertaintyHistogram(hist_vec, current_stack.get());
        if (total_hist) {
            this->SetErrorHistogramStyle(total_hist);
            total_hist->Draw("E2 SAME");
            current_total_hist.reset(total_hist); 
        }

        if (current_stack->GetHistogram()) {
            double current_max = current_stack->GetMaximum();
            if (current_stack->GetStack() && current_stack->GetStack()->Last()) {
                TH1* sum_hist_for_max = static_cast<TH1*>(current_stack->GetStack()->Last());
                for (int i = 1; i <= sum_hist_for_max->GetNbinsX(); ++i) {
                    current_max = std::max(current_max, sum_hist_for_max->GetBinContent(i) + sum_hist_for_max->GetBinError(i));
                }
            }
            if (current_max > 0) {
                current_stack->SetMaximum(current_max * 1.35);
            } else if (current_max == 0 && current_stack->GetMinimum() == 0) {
                current_stack->SetMaximum(1.0);
            }
            current_stack->SetMinimum(0.0);
        }

        canvas->Modified();
        canvas->Update();
    }
};

}
#endif