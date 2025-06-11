#ifndef PLOTRATIO_H
#define PLOTRATIO_H

#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <tuple>

#include "PlotBase.h"
#include "AnalysisResult.h"
#include "Histogram.h"
#include "TCanvas.h"
#include "TPad.h"
#include "TLegend.h"
#include "TGraphAsymmErrors.h"
#include "TLatex.h"
#include "TH1D.h"
#include "TLine.h"
#include "TStyle.h"
#include "TMatrixDSym.h"
#include "TVectorD.h"
#include "AnalysisChannels.h"

namespace AnalysisFramework {

inline std::pair<double, int> calculateChi2(const TH1D* h_Pred, const TH1D* h_Data, const TMatrixDSym* cov_sys = nullptr, const std::vector<int>& skip = {}) {
    if (!h_Pred || !h_Data) {
        std::cerr << "Warning: calculateChi2 received null histograms." << std::endl;
        return {0.0, 0};
    }

    const int nbins = h_Data->GetNbinsX();
    std::vector<int> nonzero_bins;
    for (int i = 1; i < nbins + 1; ++i) {
        if (h_Data->GetBinContent(i) > 0 && h_Pred->GetBinContent(i) > 0 && std::find(skip.begin(), skip.end(), i) == skip.end()) {
            nonzero_bins.push_back(i);
        }
    }

    if (nonzero_bins.empty()) {
        return {0.0, 0};
    }

    TMatrixDSym cov_full(nonzero_bins.size());
    cov_full.Zero();

    if (cov_sys && cov_sys->GetNcols() == nbins) {
        for (size_t i = 0; i < nonzero_bins.size(); ++i) {
            for (size_t j = 0; j < nonzero_bins.size(); ++j) {
                cov_full[i][j] = (*cov_sys)[nonzero_bins.at(i) - 1][nonzero_bins.at(j) - 1];
            }
        }
    }

    TMatrixDSym cov_stat(nonzero_bins.size());
    cov_stat.Zero();
    for (size_t i = 0; i < nonzero_bins.size(); ++i) {
        double pred_error_sq = h_Pred->GetBinError(nonzero_bins.at(i)) * h_Pred->GetBinError(nonzero_bins.at(i));
        double data_error_sq = h_Data->GetBinError(nonzero_bins.at(i)) * h_Data->GetBinError(nonzero_bins.at(i));
        cov_stat[i][i] = pred_error_sq + data_error_sq;
    }
    cov_full += cov_stat;

    if (cov_full.Determinant() == 0) {
        std::cerr << "Warning: Covariance matrix is singular or near-singular. Chi2 calculation skipped." << std::endl;
        return {0.0, 0};
    }

    cov_full.Invert();

    double chi2 = 0.0;
    for (size_t i = 0; i < nonzero_bins.size(); ++i) {
        for (size_t j = 0; j < nonzero_bins.size(); ++j) {
            double diff_i = h_Pred->GetBinContent(nonzero_bins.at(i)) - h_Data->GetBinContent(nonzero_bins.at(i));
            double diff_j = h_Pred->GetBinContent(nonzero_bins.at(j)) - h_Data->GetBinContent(nonzero_bins.at(j));
            chi2 += diff_i * diff_j * cov_full[i][j];
        }
    }
    return {chi2, static_cast<int>(nonzero_bins.size())};
}


class PlotRatio : public PlotBase {
public:
    inline PlotRatio(const std::string& name, const AnalysisResult& result, const std::string& analysis_channel_column, const std::string& output_dir = "plots")
        : PlotBase(name, output_dir), result_(result), analysis_channel_column_(analysis_channel_column),
          mc_stack_(nullptr), legend_(nullptr), h_errors_ratio_(nullptr), h_data_ratio_(nullptr) {}

    inline ~PlotRatio() {
        delete mc_stack_;
        delete legend_;
        delete h_errors_ratio_;
        delete h_data_ratio_;
    }

protected:
    inline void draw(TCanvas& canvas) override {
        const double Dual_CanvasX = 800;
        const double Dual_CanvasY = 750;
        const double Dual_PadSplitLow = 0.3;
        const double Dual_PadSplitHigh = 0.9;

        const double Dual_MainXaxisTitleSize = 0;
        const double Dual_MainYaxisTitleSize = 0.065;
        const double Dual_MainXaxisLabelSize = 0;
        const double Dual_MainYaxisLabelSize = 0.05;
        const double Dual_MainYaxisTitleOffset = 0.8;

        const double Dual_RatioXaxisTitleSize = 0.12;
        const double Dual_RatioYaxisTitleSize = 0.12;
        const double Dual_RatioXaxisTitleOffset = 0.9;
        const double Dual_RatioYaxisTitleOffset = 0.43;
        const double Dual_RatioXaxisLabelSize = 0.1;
        const double Dual_RatioYaxisLabelSize = 0.1;
        const double Dual_TextLabelSize = 0.17;

        canvas.SetCanvasSize(Dual_CanvasX, Dual_CanvasY);
        canvas.cd();

        TPad *p_legend = new TPad("legend_pad", "legend_pad", 0.0, Dual_PadSplitHigh, 1.0, 1.0);
        TPad *p_main = new TPad("main_pad", "main_pad", 0.0, Dual_PadSplitLow, 1.0, Dual_PadSplitHigh);
        TPad *p_ratio = new TPad("ratio_pad", "ratio_pad", 0.0, 0.0, 1.0, Dual_PadSplitLow);

        p_legend->SetBottomMargin(0);
        p_legend->SetTopMargin(0.1);
        p_main->SetTopMargin(0.01);
        p_main->SetBottomMargin(0.02);
        p_ratio->SetTopMargin(0.005);
        p_ratio->SetBottomMargin(0.22);
        p_ratio->SetGrid(0, 1);

        p_legend->Draw();
        p_main->Draw();
        p_ratio->Draw();

        p_legend->cd();
        legend_ = new TLegend(0.1, 0.0, 0.9, 1.0);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);

        mc_stack_ = new THStack("mc_stack", "");
        std::vector<Histogram> mc_hists;
        for (auto const& [key, val] : result_.getHistBreakdown()) {
            mc_hists.push_back(val);
        }
        std::sort(mc_hists.begin(), mc_hists.end(), [](const Histogram& a, const Histogram& b) {
            return a.sum() < b.sum();
        });
        std::reverse(mc_hists.begin(), mc_hists.end());

        const auto& label_map = getChannelLabelMap().at(analysis_channel_column_);
        for (const auto& hist : mc_hists) {
            TH1D* h_leg = new TH1D();
            int channel_key = -1;
            for (const auto& [key, label] : label_map) {
                if (TString(label) == hist.GetTitle()) {
                    channel_key = key;
                    break;
                }
            }
            if (channel_key != -1) {
                setChannelHistogramStyle(analysis_channel_column_, channel_key, h_leg);
                h_leg->SetLineColor(kBlack);
                h_leg->SetLineWidth(1.5);
                std::string base_label = getChannelLabel(analysis_channel_column_, channel_key);
                std::string legend_label = base_label + " : " + formatDouble(hist.sum(), 2);
                legend_->AddEntry(h_leg, legend_label.c_str(), "f"); // Corrected: .c_str()
                delete h_leg;
            }
        }
        
        TH1D* data_hist = result_.getDataHist().getRootHistCopy("data_hist_display");
        if (!result_.isBlinded() && data_hist) {
            data_hist->SetLineWidth(1);
            data_hist->SetLineColor(kBlack);
            data_hist->SetMarkerStyle(20);
            data_hist->SetMarkerColor(kBlack);
            legend_->AddEntry(data_hist, ("Data = " + formatDouble(data_hist->Integral(), 1)).c_str(), "P"); // Corrected: .c_str()
        }

        TH1D* h_unc_legend = new TH1D();
        h_unc_legend->SetFillColor(kBlack);
        h_unc_legend->SetFillStyle(3004);
        h_unc_legend->SetLineColor(kBlack);
        h_unc_legend->SetLineWidth(1);
        legend_->AddEntry(h_unc_legend, "Total MC Uncertainty", "f");
        delete h_unc_legend;

        const int n_entries = mc_hists.size() + (result_.isBlinded() ? 0 : 1) + 1;
        int n_cols = 2;
        if(n_entries > 4) n_cols = 3;
        if(n_entries > 6) n_cols = 4;
        if(n_entries > 12) n_cols = 5;
        legend_->SetNColumns(n_cols);
        legend_->Draw();

        p_main->cd();
        TH1D* total_mc_nominal_hist = result_.getTotalHist().getRootHistCopy("total_mc_nominal_hist");
        if (total_mc_nominal_hist) {
            total_mc_nominal_hist->SetDirectory(0);
        }

        for (const auto& hist : mc_hists) {
            TH1D* h = hist.getRootHistCopy();
            int channel_key = -1;
            for (const auto& [key, label] : label_map) {
                if (TString(label) == hist.GetTitle()) {
                    channel_key = key;
                    break;
                }
            }
            if (channel_key != -1) {
                setChannelHistogramStyle(analysis_channel_column_, channel_key, h);
                h->SetLineColor(kBlack);
                h->SetLineWidth(1);
            }
            mc_stack_->Add(h, "HIST");
        }

        double max_y = total_mc_nominal_hist ? total_mc_nominal_hist->GetMaximum() + total_mc_nominal_hist->GetBinError(total_mc_nominal_hist->GetMaximumBin()) : 1.0;
        if (!result_.isBlinded() && data_hist) {
            max_y = std::max(max_y, data_hist->GetMaximum() + data_hist->GetBinError(data_hist->GetMaximumBin()));
        }
        
        mc_stack_->Draw("HIST");
        mc_stack_->SetMaximum(max_y * 1.25);
        mc_stack_->SetMinimum(0.0);
        
        if (total_mc_nominal_hist) {
            total_mc_nominal_hist->SetFillColor(kBlack);
            total_mc_nominal_hist->SetFillStyle(3004);
            total_mc_nominal_hist->SetMarkerSize(0);
            total_mc_nominal_hist->Draw("E2 SAME");
        }
        
        if (!result_.isBlinded() && data_hist) {
            data_hist->Draw("E0 P0 SAME");
        }

        TH1* frame = mc_stack_->GetHistogram();
        if (!result_.getHistBreakdown().empty()) {
            const auto& first_hist = result_.getHistBreakdown().begin()->second;
            frame->GetXaxis()->SetTitle(first_hist.binning_def.variable_tex.Data());
        }
        frame->GetYaxis()->SetTitle("Events");

        frame->GetXaxis()->SetTitleSize(Dual_MainXaxisTitleSize);
        frame->GetYaxis()->SetTitleSize(Dual_MainYaxisTitleSize);
        frame->GetXaxis()->SetLabelSize(Dual_MainXaxisLabelSize);
        frame->GetYaxis()->SetLabelSize(Dual_MainYaxisLabelSize);
        frame->GetYaxis()->SetTitleOffset(Dual_MainYaxisTitleOffset);
        frame->GetXaxis()->SetLabelOffset(999);
        frame->GetXaxis()->SetTickLength(0);
        
        p_main->SetTickx(1);
        p_main->SetTicky(1);
        p_main->RedrawAxis();

        p_ratio->cd();
        TH1D* h_mc_ratio_band = (TH1D*)total_mc_nominal_hist->Clone("h_mc_ratio_band");
        h_mc_ratio_band->SetDirectory(0);
        TH1D* h_data_ratio_plot = nullptr;

        if (!result_.isBlinded() && data_hist) {
            h_data_ratio_plot = (TH1D*)data_hist->Clone("h_data_ratio_plot");
            h_data_ratio_plot->SetDirectory(0);

            double min_ratio = 0.5, max_ratio = 1.5;

            for (int i = 1; i <= h_mc_ratio_band->GetNbinsX(); ++i) {
                double mc_content = h_mc_ratio_band->GetBinContent(i);
                double mc_error = h_mc_ratio_band->GetBinError(i);
                double data_content = h_data_ratio_plot->GetBinContent(i);
                double data_error = h_data_ratio_plot->GetBinError(i);

                if (mc_content > 0) {
                    h_mc_ratio_band->SetBinContent(i, 1.0);
                    h_mc_ratio_band->SetBinError(i, mc_error / mc_content);

                    double ratio_val = data_content / mc_content;
                    double ratio_error = data_error / mc_content;
                    h_data_ratio_plot->SetBinContent(i, ratio_val);
                    h_data_ratio_plot->SetBinError(i, ratio_error);

                    min_ratio = std::min(min_ratio, ratio_val - ratio_error - h_mc_ratio_band->GetBinError(i));
                    max_ratio = std::max(max_ratio, ratio_val + ratio_error + h_mc_ratio_band->GetBinError(i));

                } else {
                    h_mc_ratio_band->SetBinContent(i, 1.0);
                    h_mc_ratio_band->SetBinError(i, 0.0);
                    h_data_ratio_plot->SetBinContent(i, 1.0);
                    h_data_ratio_plot->SetBinError(i, 0.0);
                }
            }
            h_mc_ratio_band->SetFillColor(kBlack);
            h_mc_ratio_band->SetFillStyle(3004);
            h_mc_ratio_band->SetMarkerSize(0);
            h_mc_ratio_band->Draw("E2");
            h_mc_ratio_band->GetYaxis()->SetRangeUser(std::max(0.0, min_ratio - 0.05), max_ratio + 0.05);

            h_data_ratio_plot->SetMarkerStyle(20);
            h_data_ratio_plot->SetMarkerColor(kBlack);
            h_data_ratio_plot->SetLineColor(kBlack);
            h_data_ratio_plot->Draw("E0 P0 SAME");

            TLine *line_at_one = new TLine(h_mc_ratio_band->GetXaxis()->GetXmin(), 1.0, h_mc_ratio_band->GetXaxis()->GetXmax(), 1.0);
            line_at_one->SetLineColor(kRed);
            line_at_one->SetLineStyle(kDashed);
            line_at_one->SetLineWidth(2);
            line_at_one->Draw("SAME");
            delete line_at_one;

        } else {
            h_mc_ratio_band->SetBinContent(1,1);
            h_mc_ratio_band->SetBinError(1,0);
            h_mc_ratio_band->Draw("AXIS");
            h_mc_ratio_band->GetYaxis()->SetRangeUser(0.5, 1.5);
        }

        h_mc_ratio_band->GetXaxis()->SetTitle(frame->GetXaxis()->GetTitle());
        h_mc_ratio_band->GetYaxis()->SetTitle("Data/MC");

        h_mc_ratio_band->GetXaxis()->SetTitleSize(Dual_RatioXaxisTitleSize);
        h_mc_ratio_band->GetYaxis()->SetTitleSize(Dual_RatioYaxisTitleSize);
        h_mc_ratio_band->GetXaxis()->SetLabelSize(Dual_RatioXaxisLabelSize);
        h_mc_ratio_band->GetYaxis()->SetLabelSize(Dual_RatioYaxisLabelSize);
        h_mc_ratio_band->GetXaxis()->SetTitleOffset(Dual_RatioXaxisTitleOffset);
        h_mc_ratio_band->GetYaxis()->SetTitleOffset(Dual_RatioYaxisTitleOffset);
        h_mc_ratio_band->SetStats(0);

        if (!result_.getHistBreakdown().empty()) {
            const auto& first_hist = result_.getHistBreakdown().begin()->second;
            if (!first_hist.binning_def.bin_edges.empty() && first_hist.binning_def.nBins() > 0) {
                 for (int i = 1; i <= h_mc_ratio_band->GetNbinsX(); ++i) {
                     TString bin_label = TString::Format("[%.1f, %.1f]", first_hist.binning_def.bin_edges[i-1], first_hist.binning_def.bin_edges[i]);
                     h_mc_ratio_band->GetXaxis()->SetBinLabel(i, bin_label.Data());
                 }
                 h_mc_ratio_band->GetXaxis()->LabelsOption("v");
                 h_mc_ratio_band->GetXaxis()->SetLabelSize(Dual_TextLabelSize);
            }
        }
        
        p_ratio->SetTickx(1);
        p_ratio->SetTicky(1);
        p_ratio->RedrawAxis();

        canvas.cd();
        auto watermark_lines = formatWatermarkText();
        const double x_pos = 1.0 - p_main->GetRightMargin() - 0.03;
        const double y_pos_start = Dual_PadSplitHigh - p_main->GetTopMargin() - 0.03;

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

        if (!result_.isBlinded() && data_hist && total_mc_nominal_hist) {
            TMatrixDSym total_syst_cov_matrix;
            for(const auto& pair : result_.getSystematicBreakdown()){
                if (total_syst_cov_matrix.GetNrows() == 0) {
                    total_syst_cov_matrix.ResizeTo(pair.second.GetNrows(), pair.second.GetNcols());
                    total_syst_cov_matrix.Zero();
                }
                total_syst_cov_matrix += pair.second;
            }
            std::pair<double, int> chi2_ndof = calculateChi2(total_mc_nominal_hist, data_hist, &total_syst_cov_matrix);
            if (chi2_ndof.second > 0) {
                TLatex chi2_latex;
                chi2_latex.SetNDC();
                chi2_latex.SetTextAlign(11);
                chi2_latex.SetTextFont(42);
                chi2_latex.SetTextSize(0.05);
                chi2_latex.DrawLatex(p_main->GetLeftMargin() + 0.03, y_pos_start, TString::Format("#chi^{2}/ndof = %.1f/%d", chi2_ndof.first, chi2_ndof.second).Data());
            }
        }
        delete data_hist;
        delete total_mc_nominal_hist;
    }

private:
    AnalysisResult result_;
    std::string analysis_channel_column_;
    THStack* mc_stack_;
    TLegend* legend_;
    TH1D* h_errors_ratio_;
    TH1D* h_data_ratio_;

    std::string formatDouble(double val, int precision) const {
        std::stringstream stream;
        stream << std::fixed << std::setprecision(precision) << val;
        return stream.str();
    }

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
        if (!result_.isBlinded()) { 
             line1 = "#bf{#muBooNE Data, Preliminary}";
        }


        std::string line2 = beam_name + ", " + run_str + " (" + pot_str + ")";
        
        std::string line3 = "";
        if (!result_.getHistBreakdown().empty()) {
            const auto& binning_def = result_.getTotalHist().binning_def;
            std::string region_str = binning_def.selection_tex.Data();
            line3 = "Analysis Region: " + region_str;
        }
        
        return {line1, line2, line3};
    }
};

}

#endif