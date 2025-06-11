#ifndef PLOTEFFICIENCY_H
#define PLOTEFFICIENCY_H

#include <string>
#include <vector>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <iomanip> 
#include <sstream> 

#include "PlotBase.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TPad.h"
#include "TH1D.h"
#include "TEfficiency.h" 
#include "TGaxis.h" 
#include "TLatex.h" 
#include "TStyle.h" 

namespace AnalysisFramework {

class PlotEfficiency : public PlotBase {
public:
    inline PlotEfficiency(const std::string& name, TH1D* h_total, TH1D* h_passed, const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), h_total_(h_total), h_passed_(h_passed), efficiency_graph_(nullptr) {
        if (!h_total_ || !h_passed_) {
            throw std::runtime_error("PlotEfficiency: Total or Passed histogram is null.");
        }
        if (h_total_->GetNbinsX() != h_passed_->GetNbinsX()) {
            throw std::runtime_error("PlotEfficiency: Total and Passed histograms must have the same number of bins.");
        }
    }

    inline ~PlotEfficiency() {
        delete efficiency_graph_;
    }

protected:
    inline void draw(TCanvas& canvas) override {
        const double Single_CanvasX = 800;
        const double Single_CanvasY = 600;
        const double Single_PadSplit = 0.85; 

        const double Single_XaxisTitleSize = 0.05;
        const double Single_YaxisTitleSize = 0.05;
        const double Single_XaxisTitleOffset = 0.93;
        const double Single_YaxisTitleOffset = 1.06;
        const double Single_XaxisLabelSize = 0.045;
        const double Single_YaxisLabelSize = 0.045;
        const double Single_TextLabelSize = 0.09;

        canvas.SetCanvasSize(Single_CanvasX, Single_CanvasY);
        canvas.cd();

        TPad *p_plot = new TPad("plot_pad", "plot_pad", 0.0, 0.0, 1.0, Single_PadSplit);
        TPad *p_legend = new TPad("legend_pad", "legend_pad", 0.0, Single_PadSplit, 1.0, 1.0);
        
        p_legend->SetBottomMargin(0);
        p_legend->SetTopMargin(0.1);
        p_plot->SetTopMargin(0.01);
        p_plot->SetBottomMargin(0.12);
        p_plot->SetLeftMargin(0.12);
        p_plot->SetRightMargin(0.18); 

        p_legend->Draw();
        p_plot->Draw();

        p_legend->cd();
        TLegend *legend = new TLegend(0.1, 0.0, 0.9, 1.0);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->SetNColumns(2);

        TH1D *h_total_leg = (TH1D*)h_total_->Clone("h_total_leg");
        h_total_leg->SetLineWidth(2);
        h_total_leg->SetLineColor(kBlack); 
        legend->AddEntry(h_total_leg, "All Events", "L");
        delete h_total_leg;

        TH1D *h_passed_leg = (TH1D*)h_passed_->Clone("h_passed_leg");
        h_passed_leg->SetLineWidth(2);
        h_passed_leg->SetLineColor(kGreen + 1); 
        legend->AddEntry(h_passed_leg, "Selected Events", "L");
        delete h_passed_leg;

        TEfficiency* efficiency_obj = new TEfficiency(*h_passed_, *h_total_);
        efficiency_obj->SetConfidenceLevel(0.68);
        efficiency_obj->SetStatisticOption(TEfficiency::kFNormal); 
        efficiency_obj->SetPosteriorMode(); 

        TGraphAsymmErrors *g_eff_leg = new TGraphAsymmErrors(1);
        g_eff_leg->SetLineColor(kRed);
        g_eff_leg->SetMarkerStyle(20);
        g_eff_leg->SetMarkerSize(1.2);
        g_eff_leg->SetMarkerColor(kRed);
        g_eff_leg->SetLineWidth(2);
        legend->AddEntry(g_eff_leg, "Efficiency", "LP");
        delete g_eff_leg;

        legend->Draw();

        p_plot->cd();

        h_total_->SetLineWidth(2);
        h_total_->SetLineColor(kBlack);
        h_total_->SetFillStyle(0);
        
        h_passed_->SetLineWidth(2);
        h_passed_->SetLineColor(kGreen + 1);
        h_passed_->SetFillStyle(0);

        double max_events = std::max(h_total_->GetMaximum(), h_passed_->GetMaximum()) * 1.25;

        h_total_->SetTitle(""); 
        h_total_->GetYaxis()->SetTitle("Events");
        h_total_->SetMaximum(max_events);
        h_total_->SetMinimum(0.0);
        h_total_->Draw("HIST E0"); 

        h_passed_->Draw("HIST E0 SAME"); 

        h_total_->GetXaxis()->SetTitleSize(Single_XaxisTitleSize);
        h_total_->GetYaxis()->SetTitleSize(Single_YaxisTitleSize);
        h_total_->GetXaxis()->SetLabelSize(Single_XaxisLabelSize);
        h_total_->GetYaxis()->SetLabelSize(Single_YaxisLabelSize);
        h_total_->GetXaxis()->SetTitleOffset(Single_XaxisTitleOffset);
        h_total_->GetYaxis()->SetTitleOffset(Single_YaxisTitleOffset);
        h_total_->SetStats(0);

        std::vector<double> eff_x, eff_cv, eff_low, eff_high;
        double eff_max_val = 0.0;
        for (int i = 1; i <= h_total_->GetNbinsX(); ++i) {
            if (h_total_->GetBinContent(i) > 0) { 
                eff_x.push_back(h_total_->GetBinCenter(i));
                eff_cv.push_back(efficiency_obj->GetEfficiency(i));
                eff_low.push_back(efficiency_obj->GetEfficiencyErrorLow(i));
                eff_high.push_back(efficiency_obj->GetEfficiencyErrorUp(i));
                eff_max_val = std::max(eff_max_val, eff_cv.back() + eff_high.back());
            }
        }
        
        efficiency_graph_ = new TGraphAsymmErrors(eff_x.size(), eff_x.data(), eff_cv.data(), 0, 0, eff_low.data(), eff_high.data());
        efficiency_graph_->SetLineColor(kRed);
        efficiency_graph_->SetMarkerStyle(20);
        efficiency_graph_->SetMarkerSize(1.2);
        efficiency_graph_->SetMarkerColor(kRed);
        efficiency_graph_->SetLineWidth(2);
        efficiency_graph_->Draw("PZ SAME"); 

        double right_max = 1.15; 
        if (eff_max_val > 1.0) right_max = eff_max_val * 1.15; 
        
        TGaxis *axis = new TGaxis(p_plot->GetUxmax(), p_plot->GetUymin(),
                                  p_plot->GetUxmax(), p_plot->GetUymax(),
                                  0, right_max, 510, "+L"); 
        axis->SetTitleColor(kRed);
        axis->SetLabelColor(kRed);
        axis->SetTitleSize(Single_YaxisTitleSize);
        axis->SetTitleOffset(0.9 * Single_YaxisTitleOffset);
        axis->SetLabelSize(Single_YaxisLabelSize);
        axis->SetTitle("Efficiency");
        axis->Draw();

        auto watermark_lines = formatWatermarkText();
        const double x_pos = 1.0 - p_plot->GetRightMargin() - 0.03;
        const double y_pos_start = 1.0 - p_plot->GetTopMargin() - 0.03;

        TLatex top_watermark;
        top_watermark.SetNDC();
        top_watermark.SetTextAlign(33);
        top_watermark.SetTextFont(62);
        top_watermark.SetTextSize(0.05);
        top_watermark.DrawLatex(x_pos, y_pos_start, std::get<0>(watermark_lines).c_str());

        TLatex middle_watermark;
        middle_watermark.SetNDC();
        middle_watermark.SetTextAlign(33);
        middle_watermark.SetTextFont(42);
        middle_watermark.SetTextSize(0.05 * 0.8);
        middle_watermark.DrawLatex(x_pos, y_pos_start - 0.06, std::get<1>(watermark_lines).c_str());
        
        TLatex bottom_watermark;
        bottom_watermark.SetNDC();
        bottom_watermark.SetTextAlign(33);
        bottom_watermark.SetTextFont(42);
        bottom_watermark.SetTextSize(0.05 * 0.8);
        bottom_watermark.DrawLatex(x_pos, y_pos_start - 0.12, std::get<2>(watermark_lines).c_str());
        
        p_plot->SetTickx(1);
        p_plot->SetTicky(1);
        p_plot->RedrawAxis();

        delete efficiency_obj; 
    }

private:
    TH1D* h_total_;
    TH1D* h_passed_;
    TGraphAsymmErrors* efficiency_graph_; 

    std::tuple<std::string, std::string, std::string> formatWatermarkText() const {
        std::string beam_name = "NuMI FHC"; 
        std::string run_str = "Run 1";
        std::string pot_str = "1.0 #times 10^{20} POT";
        std::string region_str = "Efficiency Selection";

        std::string line1 = "#bf{#muBooNE Simulation, Preliminary}"; 
        std::string line2 = beam_name + ", " + run_str + " (" + pot_str + ")";
        std::string line3 = "Analysis Region: " + region_str;
        
        return {line1, line2, line3};
    }
};

}

#endif