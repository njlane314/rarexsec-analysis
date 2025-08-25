#ifndef STACKED_HISTOGRAM_PLOT_H
#define STACKED_HISTOGRAM_PLOT_H

#include <algorithm>
#include <iomanip>
#include <locale>
#include <map>
#include <numeric>
#include <sstream>
#include <tuple>
#include <vector>

#include "TArrow.h"
#include "TCanvas.h"
#include "THStack.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TPad.h"

#include "AnalysisTypes.h"
#include "HistogramCut.h"
#include "HistogramPlotterBase.h"
#include "RegionAnalysis.h"
#include "StratifierRegistry.h"

namespace analysis {
class StackedHistogramPlot : public HistogramPlotterBase {
  public:
    StackedHistogramPlot(
        std::string plot_name, const VariableResult &var_result,
        const RegionAnalysis &region_info, std::string category_column,
        std::string output_directory = "plots", bool overlay_signal = true,
        std::vector<Cut> cut_list = {}, bool annotate_numbers = true,
        bool use_log_y = false, std::string y_axis_label = "Events")
        : HistogramPlotterBase(std::move(plot_name),
                               std::move(output_directory)),
          variable_result_(var_result), region_analysis_(region_info),
          category_column_(std::move(category_column)),
          overlay_signal_(overlay_signal), cuts_(cut_list),
          annotate_numbers_(annotate_numbers), use_log_y_(use_log_y),
          y_axis_label_(std::move(y_axis_label)) {}

    ~StackedHistogramPlot() override {
        delete total_mc_hist_;
        delete mc_stack_;
        delete legend_;
        for (auto vis : cut_visuals_) {
            delete vis;
        }
    }

    void addCut(const Cut &cut) { cuts_.push_back(cut); }

  private:
    void drawWatermark(TPad *pad, double total_mc_events) const {
        pad->cd();

        auto format_double = [](double val, int precision) {
            std::stringstream stream;
            stream.imbue(std::locale(""));
            stream << std::fixed << std::setprecision(precision) << val;
            return stream.str();
        };

        std::string line1 = "#bf{#muBooNE Simulation, Preliminary}";

        std::stringstream pot_stream;
        pot_stream << std::scientific << std::setprecision(2)
                   << region_analysis_.protonsOnTarget();
        std::string pot_str = pot_stream.str();
        size_t e_pos = pot_str.find('e');
        if (e_pos != std::string::npos) {
            int exponent = std::stoi(pot_str.substr(e_pos + 1));
            pot_str = pot_str.substr(0, e_pos);
            pot_str += " #times 10^{" + std::to_string(exponent) + "}";
        }

        std::map<std::string, std::string> beam_map = {
            {"numi_fhc", "NuMI FHC"}, {"numi_rhc", "NuMI RHC"}};

        std::string beam_name = region_analysis_.beamConfig();
        if (beam_map.count(beam_name)) {
            beam_name = beam_map.at(beam_name);
        }

        std::string runs_str;
        if (!region_analysis_.runNumbers().empty()) {
            for (size_t i = 0; i < region_analysis_.runNumbers().size(); ++i) {
                runs_str +=
                    region_analysis_.runNumbers()[i] +
                    (i < region_analysis_.runNumbers().size() - 1 ? ", " : "");
            }
        } else {
            runs_str = "N/A";
        }

        std::string line2 = "Beam: " + beam_name + ", Runs: " + runs_str;
        std::string line3 = "POT: " + pot_str;
        std::string line4 =
            "#it{Analysis Region}: " + region_analysis_.regionLabel();
        std::string line5 =
            "Total Simulated Events: " + format_double(total_mc_events, 2);

        TLatex watermark;
        watermark.SetNDC();
        watermark.SetTextAlign(33);
        watermark.SetTextFont(62);
        watermark.SetTextSize(0.05);
        watermark.DrawLatex(1 - pad->GetRightMargin() - 0.03,
                            1 - pad->GetTopMargin() - 0.03, line1.c_str());
        watermark.SetTextFont(42);
        watermark.SetTextSize(0.05 * 0.8);
        watermark.DrawLatex(1 - pad->GetRightMargin() - 0.03,
                            1 - pad->GetTopMargin() - 0.09, line2.c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - 0.03,
                            1 - pad->GetTopMargin() - 0.15, line3.c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - 0.03,
                            1 - pad->GetTopMargin() - 0.21, line4.c_str());
        watermark.DrawLatex(1 - pad->GetRightMargin() - 0.03,
                            1 - pad->GetTopMargin() - 0.27, line5.c_str());
    }

  protected:
    void draw(TCanvas &canvas) override {
        log::info("StackedHistogramPlot::draw", "X-axis label from result:",
                  variable_result_.binning_.getTexLabel().c_str());
        const double PLOT_LEGEND_SPLIT = 0.85;
        canvas.cd();
        TPad *p_main =
            new TPad("main_pad", "main_pad", 0.0, 0.0, 1.0, PLOT_LEGEND_SPLIT);
        TPad *p_legend = new TPad("legend_pad", "legend_pad", 0.0,
                                  PLOT_LEGEND_SPLIT, 1.0, 1.0);
        p_main->SetTopMargin(0.01);
        p_main->SetBottomMargin(0.12);
        p_main->SetLeftMargin(0.12);
        p_main->SetRightMargin(0.05);
        if (use_log_y_)
            p_main->SetLogy();
        p_legend->SetTopMargin(0.05);
        p_legend->SetBottomMargin(0.01);
        p_legend->Draw();
        p_main->Draw();

        mc_stack_ = new THStack("mc_stack", "");
        StratifierRegistry registry;

        const auto &orig_edges = variable_result_.binning_.getEdges();
        std::vector<double> adjusted_edges = orig_edges;
        if (adjusted_edges.size() >= 4) {
            double first_width = adjusted_edges[2] - adjusted_edges[1];
            double last_width = adjusted_edges[adjusted_edges.size() - 2] -
                                adjusted_edges[adjusted_edges.size() - 3];
            adjusted_edges[0] = adjusted_edges[1] - first_width;
            adjusted_edges.back() =
                adjusted_edges[adjusted_edges.size() - 2] + last_width;
        }

        std::vector<std::pair<ChannelKey, BinnedHistogram>> mc_hists;
        for (auto const &[key, hist] : variable_result_.strat_hists_) {
            if (hist.getSum() > 0) {
                mc_hists.emplace_back(key, hist);
            }
        }

        std::sort(mc_hists.begin(), mc_hists.end(),
                  [](const auto &a, const auto &b) {
                      return a.second.getSum() < b.second.getSum();
                  });
        std::reverse(mc_hists.begin(), mc_hists.end());

        double total_mc_events = 0.0;
        for (const auto &[key, hist] : mc_hists) {
            total_mc_events += hist.getSum();
        }

        p_legend->cd();
        legend_ = new TLegend(0.12, 0.0, 0.95, 1.0);
        legend_->SetBorderSize(0);
        legend_->SetFillStyle(0);
        legend_->SetTextFont(42);
        const int n_entries = mc_hists.size() + 1;
        int n_cols = (n_entries > 4) ? 3 : 2;
        legend_->SetNColumns(n_cols);

        auto format_double = [](double val, int precision) {
            std::stringstream stream;
            stream.imbue(std::locale(""));
            stream << std::fixed << std::setprecision(precision) << val;
            return stream.str();
        };

        for (const auto &[key, hist] : mc_hists) {
            TH1D *h_leg = new TH1D();
            const auto &stratum = registry.getStratumProperties(
                category_column_, std::stoi(key.str()));
            h_leg->SetFillColor(stratum.fill_colour);
            h_leg->SetFillStyle(stratum.fill_style);
            h_leg->SetLineColor(kBlack);
            h_leg->SetLineWidth(1.5);

            std::string tex_label = stratum.tex_label;
            if (tex_label == "#emptyset") {
                tex_label = "\xE2\x88\x85";
            }

            std::string legend_label =
                annotate_numbers_
                    ? tex_label + " : " + format_double(hist.getSum(), 2)
                    : tex_label;
            legend_->AddEntry(h_leg, legend_label.c_str(), "f");
        }

        if (!mc_hists.empty()) {
            TH1D *h_unc = new TH1D();
            h_unc->SetFillColor(kBlack);
            h_unc->SetFillStyle(3004);
            h_unc->SetLineColor(kBlack);
            h_unc->SetLineWidth(1);
            std::string total_label = "Stat. #oplus Syst. Unc.";
            legend_->AddEntry(h_unc, total_label.c_str(), "f");
        }
        legend_->Draw();

        p_main->cd();

        for (const auto &[key, hist] : mc_hists) {
            TH1D *h = (TH1D *)hist.get()->Clone();
            if (adjusted_edges.size() == orig_edges.size()) {
                TH1D *rebinned = (TH1D *)h->Rebin(adjusted_edges.size() - 1, "",
                                                  adjusted_edges.data());
                delete h;
                h = rebinned;
            }
            const auto &stratum = registry.getStratumProperties(
                category_column_, std::stoi(key.str()));
            h->SetFillColor(stratum.fill_colour);
            h->SetFillStyle(stratum.fill_style);
            h->SetLineColor(kBlack);
            h->SetLineWidth(1);
            mc_stack_->Add(h, "HIST");
            if (!total_mc_hist_) {
                total_mc_hist_ = (TH1D *)h->Clone("total_mc_hist");
                total_mc_hist_->SetDirectory(0);
            } else {
                total_mc_hist_->Add(h);
            }
        }

        double max_y = total_mc_hist_ ? total_mc_hist_->GetMaximum() +
                                            total_mc_hist_->GetBinError(
                                                total_mc_hist_->GetMaximumBin())
                                      : 1.0;
        mc_stack_->Draw("HIST");
        mc_stack_->SetMaximum(max_y * (use_log_y_ ? 10 : 1.3));
        mc_stack_->SetMinimum(use_log_y_ ? 0.1 : 0.0);

        if (total_mc_hist_) {
            TMatrixDSym total_syst_cov = variable_result_.total_covariance_;

            for (int i = 1; i <= total_mc_hist_->GetNbinsX(); ++i) {
                double stat_err = total_mc_hist_->GetBinError(i);
                double syst_err = (i - 1 < total_syst_cov.GetNrows())
                                      ? std::sqrt(total_syst_cov(i - 1, i - 1))
                                      : 0.0;
                double total_err =
                    std::sqrt(stat_err * stat_err + syst_err * syst_err);
                total_mc_hist_->SetBinError(i, total_err);
            }

            total_mc_hist_->SetFillColor(kBlack);
            total_mc_hist_->SetFillStyle(3004);
            total_mc_hist_->SetMarkerSize(0);
            total_mc_hist_->Draw("E2 SAME");
        }

        for (const auto &cut : cuts_) {
            double y_arrow_pos = max_y * 0.85;
            double x_range = mc_stack_->GetXaxis()->GetXmax() -
                             mc_stack_->GetXaxis()->GetXmin();
            double arrow_length = x_range * 0.04;

            TLine *line =
                new TLine(cut.threshold, 0, cut.threshold, max_y * 1.3);
            line->SetLineColor(kRed);
            line->SetLineWidth(2);
            line->SetLineStyle(kDashed);
            line->Draw("same");
            cut_visuals_.push_back(line);

            double x_start, x_end;
            if (cut.direction == CutDirection::GreaterThan) {
                x_start = cut.threshold;
                x_end = cut.threshold + arrow_length;
            } else {
                x_start = cut.threshold;
                x_end = cut.threshold - arrow_length;
            }
            TArrow *arrow = new TArrow(x_start, y_arrow_pos, x_end, y_arrow_pos,
                                       0.025, ">");
            arrow->SetLineColor(kRed);
            arrow->SetFillColor(kRed);
            arrow->SetLineWidth(2);
            arrow->Draw("same");
            cut_visuals_.push_back(arrow);
        }

        TH1 *frame = mc_stack_->GetHistogram();
        frame->GetXaxis()->SetTitle(
            variable_result_.binning_.getTexLabel().c_str());
        frame->GetYaxis()->SetTitle(y_axis_label_.c_str());
        frame->GetXaxis()->SetTitleOffset(1.0);
        frame->GetYaxis()->SetTitleOffset(1.0);

        mc_stack_->GetXaxis()->SetLimits(adjusted_edges.front(),
                                         adjusted_edges.back());
        if (orig_edges.size() >= 3) {
            std::ostringstream uf_label, of_label;
            uf_label << "<" << orig_edges[1];
            of_label << ">" << orig_edges[orig_edges.size() - 2];
            frame->GetXaxis()->ChangeLabel(1, -1, -1, -1, -1, -1,
                                           uf_label.str().c_str());
            frame->GetXaxis()->ChangeLabel(frame->GetXaxis()->GetNbins(), -1,
                                           -1, -1, -1, -1,
                                           of_label.str().c_str());
            double line_min = use_log_y_ ? 0.1 : 0.0;
            double line_max = max_y * (use_log_y_ ? 10 : 1.3);
            double tick_max = line_min + (line_max - line_min) * 0.05;
            TLine *uf_edge =
                new TLine(orig_edges[1], line_min, orig_edges[1], tick_max);
            uf_edge->SetLineColor(kGray + 2);
            uf_edge->SetLineWidth(2);
            uf_edge->Draw("same");
            cut_visuals_.push_back(uf_edge);
            TLine *of_edge =
                new TLine(orig_edges[orig_edges.size() - 2], line_min,
                          orig_edges[orig_edges.size() - 2], tick_max);
            of_edge->SetLineColor(kGray + 2);
            of_edge->SetLineWidth(2);
            of_edge->Draw("same");
            cut_visuals_.push_back(of_edge);
        }

        drawWatermark(p_main, total_mc_events);

        p_main->RedrawAxis();
        canvas.Update();
    }

  private:
    const VariableResult &variable_result_;
    const RegionAnalysis &region_analysis_;
    std::string category_column_;
    bool overlay_signal_;
    std::vector<Cut> cuts_;
    bool annotate_numbers_;
    bool use_log_y_;
    std::string y_axis_label_;
    TH1D *total_mc_hist_ = nullptr;
    THStack *mc_stack_ = nullptr;
    TLegend *legend_ = nullptr;
    std::vector<TObject *> cut_visuals_;
};

} // namespace analysis

#endif
