#ifndef STACKEDPLOT_H
#define STACKEDPLOT_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <tuple> // Required for std::tuple

#include "PlotBase.h"
#include "AnalysisRunner.h"
#include "EventCategories.h"
#include "TColor.h"
#include "TPad.h"
#include "TLegend.h"
#include "TGraphAsymmErrors.h"
#include "TLatex.h"

namespace AnalysisFramework {

class PlotStacked : public PlotBase {
public:
    inline PlotStacked(const std::string& name, const AnalysisResult& result, const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), result_(result) {}

    inline ~PlotStacked() {
        delete total_mc_hist_;
        delete mc_stack_;
        delete legend_;
    }

protected:
    inline void draw(TCanvas& canvas) override {
        const double PLOT_LEGEND_SPLIT = 0.85;
        const double MAIN_PAD_TOP_MARGIN = 0.01; 
        const double MAIN_PAD_BOTTOM_MARGIN = 0.12;
        const double MAIN_PAD_LEFT_MARGIN = 0.12;
        const double MAIN_PAD_RIGHT_MARGIN = 0.05;
        const double MAIN_X_TITLE_SIZE = 0.05;
        const double MAIN_Y_TITLE_SIZE = 0.05;
        const double MAIN_X_LABEL_SIZE = 0.04;
        const double MAIN_Y_LABEL_SIZE = 0.04;
        const double MAIN_X_TITLE_OFFSET = 1.1;
        const double MAIN_Y_TITLE_OFFSET = 1.2;
        const double LEGEND_PAD_TOP_MARGIN = 0.05;
        const double LEGEND_PAD_BOTTOM_MARGIN = 0.01; 

        canvas.cd();
        TPad* p_main = new TPad("main_pad", "main_pad", 0.0, 0.0, 1.0, PLOT_LEGEND_SPLIT);
        TPad* p_legend = new TPad("legend_pad", "legend_pad", 0.0, PLOT_LEGEND_SPLIT, 1.0, 1.0);
        p_main->SetTopMargin(MAIN_PAD_TOP_MARGIN);
        p_main->SetBottomMargin(MAIN_PAD_BOTTOM_MARGIN);
        p_main->SetLeftMargin(MAIN_PAD_LEFT_MARGIN);
        p_main->SetRightMargin(MAIN_PAD_RIGHT_MARGIN);
        p_legend->SetTopMargin(LEGEND_PAD_TOP_MARGIN);
        p_legend->SetBottomMargin(LEGEND_PAD_BOTTOM_MARGIN);
        p_legend->Draw();
        p_main->Draw();

        mc_stack_ = new THStack("mc_stack", "");
        std::vector<Histogram> mc_hists;
        for (auto const& [key, val] : result_.getHistBreakdown()) {
            mc_hists.push_back(val);
        }
        std::sort(mc_hists.begin(), mc_hists.end(), [](const Histogram& a, const Histogram& b) {
            return a.sum() < b.sum();
        });
        std::reverse(mc_hists.begin(), mc_hists.end());
        
        double total_mc_events = 0.0;
        for(const auto& hist : mc_hists) {
            total_mc_events += hist.sum();
        }

        p_legend->cd();
        legend_ = new TLegend(0.1, 0.0, 0.9, 1.0);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);
        const int n_entries = mc_hists.size() + 1;
        int n_cols = (n_entries > 4) ? 3 : 2;
        legend_->SetNColumns(n_cols);
        auto format_double = [](double val, int precision) {
            std::stringstream stream;
            stream << std::fixed << std::setprecision(precision) << val;
            return stream.str();
        };
        const auto& label_map = GetLabelMaps().at("analysis_channel");
        for (const auto& hist : mc_hists) {
            TH1D* h_leg = new TH1D();
            int category_id = -1;
            for (const auto& [id, label] : label_map) {
                if (TString(label) == hist.GetTitle()) {
                    category_id = id;
                    break;
                }
            }
            if (category_id != -1) {
                SetHistogramStyle("analysis_channel", category_id, h_leg);
                h_leg->SetLineColor(kBlack);
                h_leg->SetLineWidth(1.5);
                std::string base_label = GetLabel("analysis_channel", category_id);
                std::string legend_label = base_label + " : " + format_double(hist.sum(), 2);
                legend_->AddEntry(h_leg, legend_label.c_str(), "f");
            }
        }
        if (!mc_hists.empty()) {
            TH1D* h_unc = new TH1D();
            h_unc->SetFillColor(kBlack);
            h_unc->SetFillStyle(3004);
            h_unc->SetLineColor(kBlack);
            h_unc->SetLineWidth(1);
            std::string total_label = "Stat. : " + format_double(total_mc_events, 2);
            legend_->AddEntry(h_unc, total_label.c_str(), "f");
        }
        legend_->Draw();

        p_main->cd();
        for (const auto& hist : mc_hists) {
            TH1D* h = hist.getRootHistCopy();
            int category_id = -1;
            for (const auto& [id, label] : label_map) {
                if (TString(label) == hist.GetTitle()) {
                    category_id = id;
                    break;
                }
            }
            if (category_id != -1) {
                SetHistogramStyle("analysis_channel", category_id, h);
                h->SetLineColor(kBlack);
                h->SetLineWidth(1);
            }
            mc_stack_->Add(h, "HIST");
            if (!total_mc_hist_) {
                total_mc_hist_ = (TH1D*)h->Clone("total_mc_hist");
                total_mc_hist_->SetDirectory(0);
            } else {
                total_mc_hist_->Add(h);
            }
        }

        double max_y = total_mc_hist_ ? total_mc_hist_->GetMaximum() + total_mc_hist_->GetBinError(total_mc_hist_->GetMaximumBin()) : 1.0;
        mc_stack_->Draw("HIST");
        mc_stack_->SetMaximum(max_y * 1.3);
        mc_stack_->SetMinimum(0.0);
        if (total_mc_hist_) {
            total_mc_hist_->SetFillColor(kBlack);
            total_mc_hist_->SetFillStyle(3004);
            total_mc_hist_->SetMarkerSize(0);
            total_mc_hist_->Draw("E2 SAME");
        }
        
        TH1* frame = mc_stack_->GetHistogram();
        frame->GetXaxis()->SetTitleSize(MAIN_X_TITLE_SIZE);
        frame->GetYaxis()->SetTitleSize(MAIN_Y_TITLE_SIZE);
        frame->GetXaxis()->SetLabelSize(MAIN_X_LABEL_SIZE);
        frame->GetYaxis()->SetLabelSize(MAIN_Y_LABEL_SIZE);
        frame->GetXaxis()->SetTitleOffset(MAIN_X_TITLE_OFFSET);
        frame->GetYaxis()->SetTitleOffset(MAIN_Y_TITLE_OFFSET);
        if (!result_.getHistBreakdown().empty()) {
            const auto& first_hist = result_.getHistBreakdown().begin()->second;
            mc_stack_->GetXaxis()->SetTitle(first_hist.binning_def.variable_tex.Data());
        }
        mc_stack_->GetYaxis()->SetTitle("Events");
        
        auto watermark_lines = formatWatermarkText();
        const double x_pos = 1.0 - MAIN_PAD_RIGHT_MARGIN - 0.03;
        const double y_pos_start = 1.0 - MAIN_PAD_TOP_MARGIN - 0.03;

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
        
        p_main->SetTickx(0);
        p_main->SetTicky(0);
        p_main->RedrawAxis();
        canvas.Update();
    }

private:
    std::tuple<std::string, std::string, std::string> formatWatermarkText() const {
        std::map<std::string, std::string> beam_map = {
            {"numi_fhc", "NuMI FHC"},
            {"numi_rhc", "NuMI RHC"},
            {"bnb", "BNB"}
        };
        std::string beam_name = beam_map.count(result_.getBeamKey()) ? beam_map.at(result_.getBeamKey()) : "Unknown Beam";

        const auto& runs = result_.getRuns();
        std::string run_str;
        if (runs.empty()) {
            run_str = "All Runs";
        } else {
            std::string run_prefix = (runs.size() > 1) ? "Runs " : "Run ";
            std::string run_list = std::accumulate(
                runs.begin() + 1, runs.end(), runs[0],
                [](const std::string& a, const std::string& b) {
                    return a.substr(3) + "+" + b.substr(3);
                });
            if (runs.size() == 1) {
                run_list = runs[0].substr(3);
            }
            run_str = run_prefix + run_list;
        }

        double pot = result_.getPOT();
        std::stringstream pot_stream;
        pot_stream << std::scientific << std::setprecision(2) << pot;
        std::string pot_str = pot_stream.str();
        
        size_t e_pos = pot_str.find('e');
        if (e_pos != std::string::npos) {
            pot_str.replace(e_pos, 1, " #times 10^{");
            pot_str += "}";
        }
        pot_str += " POT";

        std::string line1 = "#bf{#muBooNE Simulation, Preliminary}";
        std::string line2 = beam_name + ", " + run_str + " (" + pot_str + ")";
        
        std::string line3 = "";
        if (!result_.getHistBreakdown().empty()) {
            const auto& binning_def = result_.getTotalHist().binning_def;
            std::string region_str = binning_def.selection_tex.Data();
            line3 = "Analysis Region: " + region_str;
        }
        
        return {line1, line2, line3};
    }

    AnalysisResult result_;
    TH1D* total_mc_hist_ = nullptr;
    THStack* mc_stack_ = nullptr;
    TLegend* legend_ = nullptr;
};

}

#endif