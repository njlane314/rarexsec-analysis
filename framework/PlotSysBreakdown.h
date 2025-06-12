#ifndef PLOTSYSTEMATICBREAKDOWN_H
#define PLOTSYSTEMATICBREAKDOWN_H

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
#include "PlotSysCorrelation.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TStyle.h"
#include "TMatrixDSym.h"

namespace AnalysisFramework {

class PlotSystematicBreakdown : public PlotBase {
public:
    inline PlotSystematicBreakdown(const std::string& name,
                                   const Histogram& nominal_hist_template,
                                   const std::map<std::string, TMatrixDSym>& systematic_cov_breakdown,
                                   const AnalysisResult& result_metadata,
                                   const std::map<std::string, std::string>& systematic_captions,
                                   const std::string& output_dir = "plots")
        : PlotBase(name, output_dir),
          nominal_hist_template_(nominal_hist_template),
          systematic_cov_breakdown_(systematic_cov_breakdown),
          result_metadata_(result_metadata),
          systematic_captions_(systematic_captions),
          legend_(nullptr) {}

    inline ~PlotSystematicBreakdown() {
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

        TMatrixDSym total_cov_matrix(nominal_hist_template_.nBins());
        total_cov_matrix.Zero();

        std::map<std::string, TH1D*> h_frac_errors_map;
        double max_frac_error = 0.0;

        for (const auto& pair : systematic_cov_breakdown_) {
            const std::string& syst_name = pair.first;
            const TMatrixDSym& cov_matrix = pair.second;

            if (cov_matrix.GetNrows() != nominal_hist_template_.nBins()) {
                std::cerr << "Warning: Covariance matrix for " << syst_name << " has incompatible dimensions. Skipping." << std::endl;
                continue;
            }

            total_cov_matrix += cov_matrix;

            TH1D* h_fe = (TH1D*)nominal_hist_template_.getRootHistCopy(TString::Format("h_FE_%s", syst_name.c_str()).Data());
            h_fe->Reset();
            for (int i_b = 1; i_b <= h_fe->GetNbinsX(); ++i_b) {
                if (cov_matrix(i_b-1, i_b-1) >= 0) {
                    h_fe->SetBinContent(i_b, std::sqrt(cov_matrix(i_b-1, i_b-1)));
                } else {
                    h_fe->SetBinContent(i_b, 0.0);
                }
            }
            h_frac_errors_map[syst_name] = h_fe;
            max_frac_error = std::max(max_frac_error, h_fe->GetMaximum());
        }

        TH1D* h_total_frac_error = (TH1D*)nominal_hist_template_.getRootHistCopy("h_Total_FracError");
        h_total_frac_error->Reset();
        for (int i_b = 1; i_b <= h_total_frac_error->GetNbinsX(); ++i_b) {
            if (total_cov_matrix(i_b-1, i_b-1) >= 0) {
                 h_total_frac_error->SetBinContent(i_b, std::sqrt(total_cov_matrix(i_b-1, i_b-1)));
            } else {
                 h_total_frac_error->SetBinContent(i_b, 0.0);
            }
        }
        max_frac_error = std::max(max_frac_error, h_total_frac_error->GetMaximum());

        p_legend->cd();
        legend_ = new TLegend(0.1, 0.0, 0.9, 1.0);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);

        int nhists_in_legend = systematic_captions_.size() + 1;
        int ncols = 2;
        if(nhists_in_legend > 6) ncols = 3;
        if(nhists_in_legend > 12) ncols = 4;
        legend_->SetNColumns(ncols);

        int color_idx_offset = 1;
        for (const auto& pair : systematic_captions_) {
            const std::string& syst_name = pair.first;
            const std::string& caption = pair.second;
            if (h_frac_errors_map.count(syst_name)) {
                TH1D* h_fe = h_frac_errors_map.at(syst_name);
                h_fe->SetLineColor(PlotBase::GoodLineColors[color_idx_offset % (sizeof(PlotBase::GoodLineColors)/sizeof(int))]);
                h_fe->SetLineWidth(2);
                h_fe->SetFillStyle(0);
                legend_->AddEntry(h_fe, caption.c_str(), "L");
                color_idx_offset++;
            }
        }

        h_total_frac_error->SetLineColor(PlotBase::GoodLineColors[0]);
        h_total_frac_error->SetLineWidth(3);
        h_total_frac_error->SetLineStyle(2);
        h_total_frac_error->SetFillStyle(0);
        legend_->AddEntry(h_total_frac_error, "Total", "L");

        legend_->Draw();

        p_plot->cd();

        h_total_frac_error->GetYaxis()->SetTitle("Uncertainty [Events]");
        h_total_frac_error->SetMaximum(max_frac_error * 1.15);
        h_total_frac_error->SetMinimum(0.0);
        h_total_frac_error->Draw("HIST");

        for (const auto& pair : systematic_captions_) {
            const std::string& syst_name = pair.first;
            if (h_frac_errors_map.count(syst_name)) {
                h_frac_errors_map.at(syst_name)->Draw("HIST same");
            }
        }
        h_total_frac_error->Draw("HIST same");

        h_total_frac_error->GetXaxis()->SetTitle(nominal_hist_template_.binning_def.variable_tex.Data());
        h_total_frac_error->GetXaxis()->SetTitleSize(Single_XaxisTitleSize);
        h_total_frac_error->GetYaxis()->SetTitleSize(Single_YaxisTitleSize);
        h_total_frac_error->GetXaxis()->SetLabelSize(Single_XaxisLabelSize);
        h_total_frac_error->GetYaxis()->SetLabelSize(Single_YaxisLabelSize);
        h_total_frac_error->GetXaxis()->SetTitleOffset(Single_XaxisTitleOffset);
        h_total_frac_error->GetYaxis()->SetTitleOffset(Single_YaxisTitleOffset);
        h_total_frac_error->SetStats(0);

        if (!nominal_hist_template_.binning_def.bin_edges.empty() && nominal_hist_template_.binning_def.nBins() > 0) {
            for (int i = 1; i <= nominal_hist_template_.binning_def.nBins(); ++i) {
                TString bin_label = TString::Format("[%.1f, %.1f]", nominal_hist_template_.binning_def.bin_edges[i-1], nominal_hist_template_.binning_def.bin_edges[i]);
                h_total_frac_error->GetXaxis()->SetBinLabel(i, bin_label.Data());
            }
            h_total_frac_error->GetXaxis()->LabelsOption("v");
            h_total_frac_error->GetXaxis()->SetLabelSize(Single_TextLabelSize);
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

        for(auto const& [key, val] : h_frac_errors_map) {
            delete val;
        }
        delete h_total_frac_error;

        if (nominal_hist_template_.nBins() > 0) {
            PlotCovariance plot_combined_cov(nominal_hist_template_, total_cov_matrix, plot_name_ + "_CombinedCov", output_dir_);
            plot_combined_cov.drawAndSave("png");
        }
    }

private:
    Histogram nominal_hist_template_;
    std::map<std::string, TMatrixDSym> systematic_cov_breakdown_;
    AnalysisResult result_metadata_;
    std::map<std::string, std::string> systematic_captions_;
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
        const auto& binning_def = nominal_hist_template_.binning_def;
        if (!binning_def.selection_tex.IsNull() && !binning_def.selection_tex.IsWhitespace()) {
            line3 = "Analysis Region: " + std::string(binning_def.selection_tex.Data());
        }
        
        return {line1, line2, line3};
    }
};

}

#endif