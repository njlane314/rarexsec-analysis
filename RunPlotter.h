#ifndef RUN_PLOTTER_H
#define RUN_PLOTTER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <cmath>
#include <cstring>
#include <numeric>

#include "TStyle.h"
#include "TCanvas.h"
#include "TH1.h"
#include "TString.h"
#include "THStack.h"
#include "TLegend.h"
#include "TColor.h"
#include "TROOT.h"
#include "TPad.h"
#include "TLine.h"
#include "TMatrixDSym.h"
#include "TH2.h"

#include "RunHistGenerator.h"
#include "Histogram.h"

namespace AnalysisFramework {

struct PlotParameters {
    std::string name = "canvas";
    int width = 800;
    int height = 600;
    std::string title = "";
    std::string x_title = "";
    std::string y_title_main = "Events / Bin";
    std::string column = "event_category";
    double data_pot = 1.0;
    std::vector<std::string> multisim_sources;
    bool plot_mc_stack = true;
    bool plot_uncertainty_breakdown = false;
    bool plot_correlation_matrix = false;
};

class RunPlotter {
public:
    RunPlotter(const RunHistGenerator& run_hist_generator)
        : run_hist_generator_(run_hist_generator) {
            this->InitialiseCommonStyle();
        }

    void Plot(const PlotParameters& params) const {
        if (params.plot_mc_stack) {
            PlotStackedMC(params);
        }
        if (params.plot_uncertainty_breakdown) {
            PlotUncertaintyBreakdown(params);
        }
        if (params.plot_correlation_matrix) {
            PlotCorrelationMatrix(params);
        }
    }

private:
    const RunHistGenerator& run_hist_generator_;

    void InitialiseCommonStyle() const {
        const int font_style = 132;
        TStyle* style = new TStyle("ConsistentStyle", "Consistent Style for Analysis Plots");
        style->SetTitleFont(font_style, "X"); style->SetTitleFont(font_style, "Y"); style->SetTitleFont(font_style, "Z");
        style->SetTitleSize(0.04, "X"); style->SetTitleSize(0.04, "Y"); style->SetTitleSize(0.04, "Z");
        style->SetLabelFont(font_style, "X"); style->SetLabelFont(font_style, "Y"); style->SetLabelFont(font_style, "Z");
        style->SetLabelSize(0.035, "X"); style->SetLabelSize(0.035, "Y"); style->SetLabelSize(0.035, "Z");
        style->SetTitleOffset(1.2, "X"); style->SetTitleOffset(1.4, "Y");
        style->SetOptStat(0); style->SetPadTickX(1); style->SetPadTickY(1);
        style->SetPadLeftMargin(0.15); style->SetPadRightMargin(0.05);
        style->SetPadTopMargin(0.07); style->SetPadBottomMargin(0.12);
        style->SetMarkerSize(1.0); style->SetCanvasColor(0);
        style->SetPadColor(0); style->SetFrameFillColor(0);
        gROOT->SetStyle("ConsistentStyle"); gROOT->ForceStyle();
    }

    void PlotStackedMC(const PlotParameters& params) const {
        TCanvas* canvas = new TCanvas(params.name.c_str(), params.title.c_str(), params.width, params.height);

        std::map<int, AnalysisFramework::Histogram> mc_hists_map =
            run_hist_generator_.GetMonteCarloHistsWithSystematics(params.multisim_sources, params.column.c_str(), params.data_pot);

        if (mc_hists_map.empty()) {
             delete canvas;
             return;
        }

        THStack* mc_stack = new THStack("mc_stack", params.title.c_str());
        AnalysisFramework::Histogram total_mc_hist;
        bool first_mc = true;

        for (const auto& pair : mc_hists_map) {
            TH1* root_hist = pair.second.getRootHistCopy();
            root_hist->SetLineColor(kBlack);
            root_hist->SetLineWidth(1);
            root_hist->SetFillColor(pair.second.plot_color_code);
            root_hist->SetFillStyle(pair.second.plot_hatch_idx > 0 ? pair.second.plot_hatch_idx : 1001);
            mc_stack->Add(root_hist, "HIST");
            if (first_mc) { total_mc_hist = pair.second; first_mc = false; }
            else { total_mc_hist = total_mc_hist + pair.second; }
        }

        canvas->cd();
        mc_stack->Draw("HIST");
        mc_stack->GetYaxis()->SetTitle(params.y_title_main.c_str());
        mc_stack->GetXaxis()->SetTitle(params.x_title.c_str());

        TH1* total_mc_root_hist = total_mc_hist.getRootHistCopy("total_mc_with_syst");
        total_mc_root_hist->SetFillColor(kBlack);
        total_mc_root_hist->SetFillStyle(3004);
        total_mc_root_hist->SetMarkerSize(0);
        total_mc_root_hist->Draw("E2 SAME");

        if(mc_stack->GetHistogram()) {
            mc_stack->GetHistogram()->GetXaxis()->SetTitle(params.x_title.c_str());
        }
        mc_stack->SetMaximum(total_mc_root_hist->GetMaximum() * 1.5);

        TLegend* legend = new TLegend(0.65, 0.7, 0.93, 0.92);
        legend->SetFillStyle(0);
        legend->SetBorderSize(0);
        for (const auto& pair : mc_hists_map) {
            legend->AddEntry(pair.second.getRootHist(), pair.second.tex_string.Data(), "f");
        }
        legend->AddEntry(total_mc_root_hist, "MC Stat #oplus Syst Unc.", "f");
        legend->Draw();

        canvas->SaveAs((params.name + ".png").c_str());
        canvas->SaveAs((params.name + ".pdf").c_str());

        delete legend;
        delete total_mc_root_hist;
        delete mc_stack;
        delete canvas;
    }

    void PlotUncertaintyBreakdown(const PlotParameters& params) const {
        TCanvas* canvas = new TCanvas((params.name + "_unc_breakdown").c_str(), "Uncertainty Breakdown", params.width, params.height);

        auto results = run_hist_generator_.GetHistogramsAndSystematicBreakdown(params.multisim_sources, params.column.c_str());
        auto nominal_hists = results.first;
        auto syst_breakdown = results.second;

        if (nominal_hists.empty()) {
            delete canvas;
            return;
        }

        AnalysisFramework::Histogram total_nominal_hist;
        bool first = true;
        for (const auto& pair : nominal_hists) {
            if(first) { total_nominal_hist = pair.second; first = false; }
            else { total_nominal_hist = total_nominal_hist + pair.second; }
        }

        int n_bins = total_nominal_hist.nBins();
        if (n_bins == 0) {
            delete canvas;
            return;
        }

        THStack* hs = new THStack("hs", ("Fractional Uncertainty Breakdown;" + params.x_title + ";Fractional Uncertainty").c_str());
        TLegend* legend = new TLegend(0.6, 0.7, 0.93, 0.92);
        legend->SetFillStyle(0);
        legend->SetBorderSize(0);

        TH1D* h_stat = new TH1D("h_stat", "Stat. Uncertainty", n_bins, total_nominal_hist.binning_def.bin_edges.data());
        TMatrixDSym stat_cov = total_nominal_hist.getCovarianceMatrix();
        for (int i = 1; i <= n_bins; ++i) {
            double cv = total_nominal_hist.getBinContent(i-1);
            if (cv > 0) h_stat->SetBinContent(i, std::sqrt(stat_cov(i-1, i-1)) / cv);
        }
        h_stat->SetFillColor(kGray + 1);
        h_stat->SetLineColor(kGray + 2);
        hs->Add(h_stat, "HIST");
        legend->AddEntry(h_stat, "Stat. Uncertainty", "f");

        int color_idx = 0;
        std::vector<int> colors = {kRed - 7, kBlue - 7, kGreen - 3, kOrange - 3, kViolet - 7, kCyan - 7, kMagenta - 7, kYellow - 7};

        for (const auto& source : params.multisim_sources) {
            TH1D* h_syst = new TH1D(source.c_str(), source.c_str(), n_bins, total_nominal_hist.binning_def.bin_edges.data());
            TMatrixDSym total_syst_cov_for_source(n_bins);
            total_syst_cov_for_source.Zero();
            for(const auto& cat_pair : syst_breakdown) {
                 if(cat_pair.second.count(source)) {
                    total_syst_cov_for_source += cat_pair.second.at(source);
                 }
            }

            for (int i = 1; i <= n_bins; ++i) {
                double cv = total_nominal_hist.getBinContent(i-1);
                if (cv > 0) h_syst->SetBinContent(i, std::sqrt(total_syst_cov_for_source(i-1, i-1)) / cv);
            }
            h_syst->SetFillColor(colors[color_idx % colors.size()]);
            h_syst->SetLineColor(TColor::GetColorDark(colors[color_idx % colors.size()]));
            hs->Add(h_syst, "HIST");
            legend->AddEntry(h_syst, source.c_str(), "f");
            color_idx++;
        }

        canvas->cd();
        hs->Draw("NOSTACK HIST");
        if (hs->GetHistogram()) {
            hs->GetHistogram()->GetYaxis()->SetTitle("Fractional Uncertainty");
            hs->GetHistogram()->GetXaxis()->SetTitle(params.x_title.c_str());
        }
        hs->SetMaximum(hs->GetMaximum("nostack") * 1.2);
        legend->Draw();

        canvas->SaveAs((params.name + "_unc_breakdown.png").c_str());
        canvas->SaveAs((params.name + "_unc_breakdown.pdf").c_str());

        delete legend;
        delete hs;
        delete canvas;
    }

    void PlotCorrelationMatrix(const PlotParameters& params) const {
        TCanvas* canvas = new TCanvas((params.name + "_corr_matrix").c_str(), "Total Correlation Matrix", 800, 700);
        canvas->SetRightMargin(0.15);
        canvas->SetLeftMargin(0.12);
        canvas->SetTopMargin(0.10);
        canvas->SetBottomMargin(0.12);

        auto hists_with_syst = run_hist_generator_.GetMonteCarloHistsWithSystematics(params.multisim_sources, params.column.c_str(), params.data_pot);
        if (hists_with_syst.empty()) {
            delete canvas;
            return;
        }

        AnalysisFramework::Histogram total_hist_with_syst;
        bool first = true;
        for (const auto& pair : hists_with_syst) {
            if(first) { total_hist_with_syst = pair.second; first = false; }
            else { total_hist_with_syst = total_hist_with_syst + pair.second; }
        }

        if (total_hist_with_syst.nBins() == 0) {
            delete canvas;
            return;
        }

        TMatrixDSym corr_matrix = total_hist_with_syst.getCorrelationMatrix();

        TH2D* h_corr = new TH2D("h_corr", ("Total Correlation Matrix;" + params.x_title + " Bin Index;" + params.x_title + " Bin Index").c_str(),
                                corr_matrix.GetNrows(), 0, corr_matrix.GetNrows(),
                                corr_matrix.GetNcols(), 0, corr_matrix.GetNcols());

        for(int i = 0; i < corr_matrix.GetNrows(); ++i) {
            for(int j = 0; j < corr_matrix.GetNcols(); ++j) {
                h_corr->SetBinContent(i + 1, j + 1, corr_matrix(i, j));
            }
        }
        h_corr->GetZaxis()->SetTitle("Correlation");
        h_corr->GetZaxis()->SetRangeUser(-1.0, 1.0);
        h_corr->Draw("COLZ");

        canvas->SaveAs((params.name + "_corr_matrix.png").c_str());
        canvas->SaveAs((params.name + "_corr_matrix.pdf").c_str());

        delete h_corr;
        delete canvas;
    }
};

}
#endif