#ifndef SIGNALCUTFLOWPLOT_H
#define SIGNALCUTFLOWPLOT_H

#include <string>
#include <vector>

#include "TColor.h"
#include "TGaxis.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"
#include "TH1F.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TVirtualPad.h"

#include <rarexsec/plot/IHistogramPlot.h>

namespace analysis {

struct CutFlowLossInfo {
  std::string reason;
  double top_count{0.0};
  double total{0.0};
};

class SignalCutFlowPlot : public IHistogramPlot {
public:
  SignalCutFlowPlot(std::string plot_name, std::vector<std::string> stages,
                    std::vector<double> survival, std::vector<double> err_low,
                    std::vector<double> err_high, double N0,
                    std::vector<double> counts,
                    std::vector<CutFlowLossInfo> losses, double pot_scale = 1.0,
                    std::string output_directory = "plots",
                    std::string x_label = "Selection stage",
                    std::string y_label = "Selection efficiency",
                    std::vector<double> mc_purity = {},
                    std::vector<double> total_purity = {},
                    std::string y2_label = "Purity (%)",
                    std::vector<double> syst_low = {},
                    std::vector<double> syst_high = {}, int band_color = kGray,
                    double band_alpha = 0.3)
      : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
        stages_(std::move(stages)), survival_(std::move(survival)),
        err_low_(std::move(err_low)), err_high_(std::move(err_high)), N0_(N0),
        counts_(std::move(counts)), losses_(std::move(losses)),
        pot_scale_(pot_scale), x_label_(std::move(x_label)),
        y_label_(std::move(y_label)), mc_purity_(std::move(mc_purity)),
        total_purity_(std::move(total_purity)), y2_label_(std::move(y2_label)),
        syst_low_(std::move(syst_low)), syst_high_(std::move(syst_high)),
        band_color_(band_color), band_alpha_(band_alpha) {}

protected:
  void draw(TCanvas &) override {
    int n = static_cast<int>(stages_.size());
    std::string title = ";" + x_label_ + ";" + y_label_;
    auto *h = new TH1F("h_surv", title.c_str(), n, 0.5, n + 0.5);
    h->SetDirectory(nullptr);
    for (int i = 0; i < n; ++i) {
      h->GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
      h->SetBinContent(i + 1, survival_[i] * 100.0);
    }
    double y_min = 0.01;
    double y_max = 100.0;
    h->SetMinimum(y_min);
    h->SetMaximum(y_max);

    // Ensure axis fonts/sizes follow global style
    h->GetXaxis()->SetLabelFont(gStyle->GetLabelFont("X"));
    h->GetYaxis()->SetLabelFont(gStyle->GetLabelFont("Y"));
    h->GetXaxis()->SetTitleFont(gStyle->GetTitleFont("X"));
    h->GetYaxis()->SetTitleFont(gStyle->GetTitleFont("Y"));
    h->GetXaxis()->SetLabelSize(gStyle->GetLabelSize("X"));
    h->GetYaxis()->SetLabelSize(gStyle->GetLabelSize("Y"));
    h->GetXaxis()->SetTitleSize(gStyle->GetTitleSize("X"));
    h->GetYaxis()->SetTitleSize(gStyle->GetTitleSize("Y"));

    h->Draw("hist");

    TGraphAsymmErrors *gb = nullptr;
    if (syst_low_.size() == static_cast<size_t>(n) &&
        syst_high_.size() == static_cast<size_t>(n)) {
      gb = new TGraphAsymmErrors(n);
      for (int i = 0; i < n; ++i) {
        gb->SetPoint(i, i + 1, survival_[i] * 100.0);
        gb->SetPointError(i, 0.0, 0.0, syst_low_[i] * 100.0,
                          syst_high_[i] * 100.0);
      }
      gb->SetFillColorAlpha(band_color_, band_alpha_);
      gb->SetLineColorAlpha(band_color_, 0.0);
      gb->Draw("2 SAME");
    }

    auto *g = new TGraphAsymmErrors(n);
    for (int i = 0; i < n; ++i) {
      g->SetPoint(i, i + 1, survival_[i] * 100.0);
      g->SetPointError(i, 0.0, 0.0, err_low_[i] * 100.0, err_high_[i] * 100.0);
    }
    g->Draw("P SAME");

    TGraph *gp_mc = nullptr;
    TGraph *gp_tot = nullptr;
    if (mc_purity_.size() == static_cast<size_t>(n) ||
        total_purity_.size() == static_cast<size_t>(n)) {
      gPad->SetRightMargin(0.15);
      if (mc_purity_.size() == static_cast<size_t>(n)) {
        gp_mc = new TGraph(n);
        gp_mc->SetLineColor(kRed);
        gp_mc->SetMarkerColor(kRed);
        gp_mc->SetMarkerStyle(24);
        for (int i = 0; i < n; ++i)
          gp_mc->SetPoint(i, i + 1, mc_purity_[i] * 100.0);
        gp_mc->Draw("PL SAME");
      }
      if (total_purity_.size() == static_cast<size_t>(n)) {
        gp_tot = new TGraph(n);
        gp_tot->SetLineColor(kBlue);
        gp_tot->SetMarkerColor(kBlue);
        gp_tot->SetMarkerStyle(25);
        for (int i = 0; i < n; ++i)
          gp_tot->SetPoint(i, i + 1, total_purity_[i] * 100.0);
        gp_tot->Draw("PL SAME");
      }

      auto *axis =
          new TGaxis(n + 0.5, h->GetMinimum(), n + 0.5, h->GetMaximum(),
                     h->GetMinimum(), h->GetMaximum(), 510, "+LG");
      axis->SetLineColor(kBlack);
      axis->SetLabelColor(kBlack);
      axis->SetTitleColor(kBlack);
      axis->SetLabelFont(gStyle->GetLabelFont("Y"));
      axis->SetTitleFont(gStyle->GetTitleFont("Y"));
      axis->SetLabelSize(gStyle->GetLabelSize("Y"));
      axis->SetTitleSize(gStyle->GetTitleSize("Y"));
      axis->SetTitle(y2_label_.c_str());
      axis->SetMoreLogLabels();
      axis->SetNoExponent();
      axis->Draw();
    }

    TLegend *legend = new TLegend(0.12, 0.75, 0.95, 0.9);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->SetTextFont(42);
    int n_entries = 1 + (gb ? 1 : 0) + (gp_mc ? 1 : 0) + (gp_tot ? 1 : 0);
    legend->SetNColumns(n_entries > 4 ? 3 : 2);
    legend->AddEntry(g, "Selection efficiency", "p");
    if (gb)
      legend->AddEntry(gb, "Syst. Unc.", "f");
    if (gp_mc)
      legend->AddEntry(gp_mc, "MC Purity", "pl");
    if (gp_tot)
      legend->AddEntry(gp_tot, "Total Purity", "pl");
    legend->Draw();

    TLatex latex;
    latex.SetTextAlign(21);
    latex.SetTextFont(h->GetXaxis()->GetTitleFont());
    latex.SetTextSize(h->GetXaxis()->GetLabelSize() * 0.5);
    for (int i = 0; i < n; ++i) {
      auto txt = TString::Format("%0.1f/%0.1f", counts_[i] * pot_scale_,
                                 N0_ * pot_scale_);
      latex.DrawLatex(i + 1, survival_[i] * 100.0 * 1.05, txt.Data());
    }

    double prev_s = 1.0;
    for (int i = 1; i < n; ++i) {
      double drop = (prev_s - survival_[i]) * 100.0;
      prev_s = survival_[i];
      const auto &info = losses_[i];
      if (drop > 0.2 && info.total > 0) {
        double frac = info.top_count > 0
                          ? static_cast<double>(info.top_count) / info.total
                          : 0.0;
        auto txt = TString::Format(
            "-%0.1f%%: %s (%0.1f/%0.1f, %0.0f%%)", drop, info.reason.c_str(),
            info.top_count * pot_scale_, info.total * pot_scale_, frac * 100.0);
        latex.DrawLatex(i + 1 - 0.1, survival_[i] * 100.0 * 1.1, txt.Data());
      }
    }
  }

private:
  std::vector<std::string> stages_;
  std::vector<double> survival_;
  std::vector<double> err_low_;
  std::vector<double> err_high_;
  double N0_;
  std::vector<double> counts_;
  std::vector<CutFlowLossInfo> losses_;
  double pot_scale_;
  std::string x_label_;
  std::string y_label_;
  std::vector<double> mc_purity_;
  std::vector<double> total_purity_;
  std::string y2_label_;
  std::vector<double> syst_low_;
  std::vector<double> syst_high_;
  int band_color_;
  double band_alpha_;
};

} // namespace analysis

#endif
