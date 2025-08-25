#ifndef UNSTACKED_HISTOGRAM_PLOT_H
#define UNSTACKED_HISTOGRAM_PLOT_H

#include <algorithm>
#include <iomanip>
#include <map>
#include <numeric>
#include <sstream>
#include <tuple>
#include <vector>
#include <locale>

#include "TArrow.h"
#include "TCanvas.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLine.h"
#include "TPad.h"

#include "AnalysisTypes.h"
#include "HistogramPlotterBase.h"
#include "RegionAnalysis.h"
#include "StratifierRegistry.h"
#include "HistogramCut.h"

namespace analysis {
class UnstackedHistogramPlot : public HistogramPlotterBase {
public:
  UnstackedHistogramPlot(std::string plot_name, const VariableResult &var_result,
                         const RegionAnalysis &region_info,
                         std::string category_column,
                         std::string output_directory = "plots",
                         std::vector<Cut> cut_list = {},
                         bool annotate_numbers = true, bool use_log_y = false,
                         std::string y_axis_label = "Events",
                         bool area_normalise = false)
      : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)),
        variable_result_(var_result), region_analysis_(region_info),
        category_column_(std::move(category_column)), cuts_(cut_list),
        annotate_numbers_(annotate_numbers), use_log_y_(use_log_y),
        y_axis_label_(std::move(y_axis_label)),
        area_normalise_(area_normalise) {}

  ~UnstackedHistogramPlot() override {
    for (auto *h : hists_) {
      delete h;
    }
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

    std::map<std::string, std::string> beam_map = {{"numi_fhc", "NuMI FHC"},
                                                   {"numi_rhc", "NuMI RHC"}};

    std::string beam_name = region_analysis_.beamConfig();
    if (beam_map.count(beam_name)) {
      beam_name = beam_map.at(beam_name);
    }

    std::string runs_str;
    if (!region_analysis_.runNumbers().empty()) {
      for (size_t i = 0; i < region_analysis_.runNumbers().size(); ++i) {
        runs_str += region_analysis_.runNumbers()[i] +
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
    log::info("UnstackedHistogramPlot::draw", "X-axis label from result:",
              variable_result_.binning_.getTexLabel().c_str());
    const double PLOT_LEGEND_SPLIT = 0.85;
    canvas.cd();
    TPad *p_main =
        new TPad("main_pad", "main_pad", 0.0, 0.0, 1.0, PLOT_LEGEND_SPLIT);
    TPad *p_legend =
        new TPad("legend_pad", "legend_pad", 0.0, PLOT_LEGEND_SPLIT, 1.0, 1.0);
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

    StratifierRegistry registry;

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
    const int n_entries = mc_hists.size();
    int n_cols = (n_entries > 4) ? 3 : 2;
    legend_->SetNColumns(n_cols);

    auto format_double = [](double val, int precision) {
      std::stringstream stream;
      stream.imbue(std::locale(""));
      stream << std::fixed << std::setprecision(precision) << val;
      return stream.str();
    };

    p_main->cd();
    double max_y = use_log_y_ ? 0.1 : 0.0;
    bool first_drawn = false;

    for (const auto &[key, hist] : mc_hists) {
      TH1D *h = (TH1D *)hist.get()->Clone();
      h->SetDirectory(0);
      const auto &stratum =
          registry.getStratumProperties(category_column_, std::stoi(key.str()));
      h->SetLineColor(stratum.fill_colour);
      h->SetLineWidth(2);
      h->SetFillStyle(0);
      if (area_normalise_ && h->Integral() > 0)
        h->Scale(1.0 / h->Integral());
      max_y = std::max(max_y, h->GetMaximum());
      if (!first_drawn) {
        h->Draw("HIST");
        first_drawn = true;
      } else {
        h->Draw("HIST SAME");
      }
      hists_.push_back(h);

      TH1D *h_leg = new TH1D();
      h_leg->SetLineColor(stratum.fill_colour);
      h_leg->SetLineWidth(2);
      h_leg->SetFillStyle(0);

      std::string tex_label = stratum.tex_label;
      if (tex_label == "#emptyset") {
        tex_label = "\xE2\x88\x85"; // UTF-8 encoding of \u2205
      }
      std::string legend_label = annotate_numbers_
                                     ? tex_label + " : " +
                                           format_double(hist.getSum(), 2)
                                     : tex_label;
      legend_->AddEntry(h_leg, legend_label.c_str(), "l");
    }

    legend_->Draw();

    for (const auto &cut : cuts_) {
      double y_arrow_pos = max_y * 0.85;
      double x_range =
          hists_.empty() ? 0.0
                         : hists_[0]->GetXaxis()->GetXmax() -
                               hists_[0]->GetXaxis()->GetXmin();
      double arrow_length = x_range * 0.04;

      TLine *line = new TLine(cut.threshold, 0, cut.threshold, max_y * 1.3);
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
      TArrow *arrow =
          new TArrow(x_start, y_arrow_pos, x_end, y_arrow_pos, 0.025, ">");
      arrow->SetLineColor(kRed);
      arrow->SetFillColor(kRed);
      arrow->SetLineWidth(2);
      arrow->Draw("same");
      cut_visuals_.push_back(arrow);
    }

    if (!hists_.empty()) {
      hists_[0]->GetXaxis()->SetTitle(
          variable_result_.binning_.getTexLabel().c_str());
      hists_[0]->GetYaxis()->SetTitle(y_axis_label_.c_str());
      hists_[0]->GetXaxis()->SetTitleOffset(1.0);
      hists_[0]->GetYaxis()->SetTitleOffset(1.0);

      const auto &edges = variable_result_.binning_.getEdges();
      hists_[0]->GetXaxis()->SetLimits(edges.front(), edges.back());
      if (edges.size() >= 3) {
        std::ostringstream uf_label, of_label;
        uf_label << "<" << edges[1];
        of_label << ">" << edges[edges.size() - 2];
        hists_[0]->GetXaxis()->ChangeLabel(1, -1, -1, -1, -1, -1,
                                           uf_label.str().c_str());
        hists_[0]->GetXaxis()->ChangeLabel(
            hists_[0]->GetXaxis()->GetNbins(), -1, -1, -1, -1, -1,
            of_label.str().c_str());
      }
    }

    drawWatermark(p_main, total_mc_events);

    p_main->RedrawAxis();
    canvas.Update();
  }

private:
  const VariableResult &variable_result_;
  const RegionAnalysis &region_analysis_;
  std::string category_column_;
  std::vector<Cut> cuts_;
  bool annotate_numbers_;
  bool use_log_y_;
  std::string y_axis_label_;
  bool area_normalise_;
  std::vector<TH1D *> hists_;
  TLegend *legend_ = nullptr;
  std::vector<TObject *> cut_visuals_;
};

} // namespace analysis

#endif
