#ifndef PLOTSYSTEMATICVARIATIONS_H
#define PLOTSYSTEMATICVARIATIONS_H

#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <map>
#include <iomanip>
#include <sstream>
#include <tuple>

#include "PlotBase.h"
#include "AnalysisResult.h"
#include "Histogram.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TH1D.h"
#include "THStack.h"
#include "TLatex.h"
#include "TStyle.h"

namespace AnalysisFramework {

class PlotSystematicVariations : public PlotBase {
public:
    inline PlotSystematicVariations(const std::string& name,
                                    const Histogram& cv_hist,
                                    const std::map<std::string, Histogram>& varied_hists,
                                    const AnalysisResult& result_metadata,
                                    const std::string& systematic_label,
                                    const std::string& output_dir = "plots")
        : PlotBase(name, output_dir),
          cv_hist_(cv_hist),
          varied_hists_(varied_hists),
          result_metadata_(result_metadata),
          systematic_label_(systematic_label),
          mc_stack_(nullptr),
          legend_(nullptr) {}

    inline ~PlotSystematicVariations() {
        delete mc_stack_;
        delete legend_;
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

        TPad *p_plot = new TPad("plot_pad", "plot_pad", 0, 0, 1, Single_PadSplit);
        TPad *p_legend = new TPad("legend_pad", "legend_pad", 0, Single_PadSplit, 1, 1);
        p_legend->SetBottomMargin(0);
        p_legend->SetTopMargin(0.1);
        p_plot->SetTopMargin(0.01);
        p_plot->SetBottomMargin(0.12);
        p_plot->SetLeftMargin(0.12);
        p_plot->SetRightMargin(0.05);

        p_legend->Draw();
        p_plot->Draw();

        p_legend->cd();
        legend_ = new TLegend(0.1, 0.0, 0.9, 1.0);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);
        legend_->SetNColumns(2);

        TH1D* h_cv_leg = cv_hist_.getRootHistCopy("h_cv_leg");
        h_cv_leg->SetLineColor(kBlack);
        h_cv_leg->SetLineWidth(2);
        h_cv_leg->SetFillColor(0);
        legend_->AddEntry(h_cv_leg, "Central Value", "L");
        delete h_cv_leg;

        std::vector<TH1D*> hists_to_draw;
        hists_to_draw.reserve(varied_hists_.size());

        if (varied_hists_.size() == 2 && varied_hists_.count("up") && varied_hists_.count("dn")) {
            TH1D* h_dn = varied_hists_.at("dn").getRootHistCopy("h_dn");
            h_dn->SetLineColor(kRed);
            hists_to_draw.push_back(h_dn);
            legend_->AddEntry(h_dn, (systematic_label_ + " -1#sigma").c_str(), "L"); // Corrected: .c_str()

            TH1D* h_up = varied_hists_.at("up").getRootHistCopy("h_up");
            h_up->SetLineColor(kBlue);
            hists_to_draw.push_back(h_up);
            legend_->AddEntry(h_up, (systematic_label_ + " +1#sigma").c_str(), "L"); // Corrected: .c_str()
        } else if (varied_hists_.size() == 1 && varied_hists_.count("var")) {
            TH1D* h_var = varied_hists_.at("var").getRootHistCopy("h_var");
            h_var->SetLineColor(kRed);
            hists_to_draw.push_back(h_var);
            legend_->AddEntry(h_var, (systematic_label_ + " Alt. Model").c_str(), "L"); // Corrected: .c_str()
        } else {
            int color_idx = 0;
            for (const auto& pair : varied_hists_) {
                TH1D* h_var = pair.second.getRootHistCopy(TString::Format("h_var_%d", color_idx).Data());
                h_var->SetLineColor(kGreen + (color_idx % 4));
                hists_to_draw.push_back(h_var);
                legend_->AddEntry(h_var, (systematic_label_ + " Var " + pair.first).c_str(), "L"); // Corrected: .c_str()
                color_idx++;
            }
            if (hists_to_draw.size() > 2) {
                legend_->SetNColumns(3);
            }
        }
        
        legend_->Draw();

        p_plot->cd();
        mc_stack_ = new THStack("mc_stack", "");
        
        for(TH1D* h_var : hists_to_draw) {
            mc_stack_->Add(h_var, "HIST");
        }
        TH1D* h_cv_plot = cv_hist_.getRootHistCopy("h_cv_plot");
        h_cv_plot->SetLineColor(kBlack);
        h_cv_plot->SetLineWidth(2);
        h_cv_plot->SetFillColor(0);
        mc_stack_->Add(h_cv_plot, "HIST");

        double maximum = h_cv_plot->GetMaximum();
        for(TH1D* h_var : hists_to_draw) {
            maximum = std::max(maximum, h_var->GetMaximum());
        }
        
        mc_stack_->Draw("HIST nostack");
        mc_stack_->SetMaximum(maximum * 1.25);
        mc_stack_->SetMinimum(0.0);

        TH1* frame = mc_stack_->GetHistogram();
        frame->GetXaxis()->SetTitle(cv_hist_.binning_def.variable_tex.Data());
        frame->GetYaxis()->SetTitle("Events");

        frame->GetXaxis()->SetTitleSize(Single_XaxisTitleSize);
        frame->GetYaxis()->SetTitleSize(Single_YaxisTitleSize);
        frame->GetXaxis()->SetLabelSize(Single_XaxisLabelSize);
        frame->GetYaxis()->SetLabelSize(Single_YaxisLabelSize);
        frame->GetXaxis()->SetTitleOffset(Single_XaxisTitleOffset);
        frame->GetYaxis()->SetTitleOffset(Single_YaxisTitleOffset);
        frame->SetStats(0);

        if (!cv_hist_.binning_def.bin_edges.empty() && cv_hist_.binning_def.nBins() > 0) {
            for (int i = 1; i <= cv_hist_.binning_def.nBins(); ++i) {
                TString bin_label = TString::Format("[%.1f, %.1f]", cv_hist_.binning_def.bin_edges[i-1], cv_hist_.binning_def.bin_edges[i]);
                mc_stack_->GetXaxis()->SetBinLabel(i, bin_label.Data());
            }
            mc_stack_->GetXaxis()->LabelsOption("v");
            mc_stack_->GetXaxis()->SetLabelSize(Single_TextLabelSize);
        }
        
        p_plot->SetTickx(1);
        p_plot->SetTicky(1);
        p_plot->RedrawAxis();

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

        for(TH1D* h_var : hists_to_draw) {
            delete h_var;
        }
        delete h_cv_plot;
    }

private:
    Histogram cv_hist_;
    std::map<std::string, Histogram> varied_hists_;
    AnalysisResult result_metadata_;
    std::string systematic_label_;
    THStack* mc_stack_;
    TLegend* legend_;

    std::tuple<std::string, std::string, std::string> formatWatermarkText() const {
        std::map<std::string, std::string> beam_map = {
            {"numi_fhc", "NuMI FHC"},
            {"numi_rhc", "NuMI RHC"},
            {"bnb", "BNB"}
        };
        std::string beam_name = beam_map.count(result_metadata_.getBeamKey()) ? beam_map.at(result_metadata_.getBeamKey()) : "Unknown Beam";

        const auto& runs = result_metadata_.getRuns();
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

        double pot = result_metadata_.getPOT();
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
        const auto& binning_def = cv_hist_.binning_def;
        if (!binning_def.selection_tex.IsNull() && !binning_def.selection_tex.IsWhitespace()) {
            line3 = "Analysis Region: " + std::string(binning_def.selection_tex.Data());
        }
        
        return {line1, line2, line3};
    }
};

}

#endif