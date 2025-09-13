#ifndef SIGNALCUTFLOWPLOT_H
#define SIGNALCUTFLOWPLOT_H

#include <string>
#include <vector>
#include <algorithm>
#include <limits>

#include "TCanvas.h"
#include "TColor.h"
#include "TGaxis.h"
#include "TGraph.h"
#include "TGraphAsymmErrors.h"
#include "TH1F.h"
#include "TLegend.h"
#include "TString.h"
#include "TStyle.h"
#include "TVirtualPad.h"
#include "TPad.h"

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
                    std::string eff_label = "Selection efficiency",
                    std::vector<double> mc_bkg_rej = {},
                    std::vector<double> total_bkg_rej = {},
                    std::string y2_label = "Background rejection (%)",
                    std::vector<double> syst_low = {},
                    std::vector<double> syst_high = {}, int band_color = kGray,
                    double band_alpha = 0.3)
      : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
        stages_(std::move(stages)), survival_(std::move(survival)),
        err_low_(std::move(err_low)), err_high_(std::move(err_high)), N0_(N0),
        counts_(std::move(counts)), losses_(std::move(losses)),
        pot_scale_(pot_scale), x_label_(std::move(x_label)),
        y_label_(std::move(y_label)), eff_label_(std::move(eff_label)),
        mc_bkg_rej_(std::move(mc_bkg_rej)),
        total_bkg_rej_(std::move(total_bkg_rej)), y2_label_(std::move(y2_label)),
        syst_low_(std::move(syst_low)), syst_high_(std::move(syst_high)),
        band_color_(band_color), band_alpha_(band_alpha) {}

protected:
  void draw(TCanvas &canvas) override {
    // Labels (you can tweak these defaults)
    x_label_ = "Selection Stage";
    y_label_ = "Selection efficiency";

    const double split = 0.85; // lower pad for plot, upper for legend

    // -- Pads -----------------------------------------------------------------
    canvas.cd();
    TPad *padMain = new TPad("padMain", "padMain", 0.0, 0.0, 1.0, split);
    padMain->SetLeftMargin(0.15);
    padMain->SetRightMargin(0.10);
    padMain->SetTopMargin(0.01);
    padMain->SetBottomMargin(0.14);
    padMain->Draw();

    TPad *padLegend = new TPad("padLegend", "padLegend", 0.0, split, 1.0, 1.0);
    padLegend->SetFillStyle(0);
    padLegend->SetFrameFillStyle(0);
    padLegend->SetLeftMargin(0.05);
    padLegend->SetRightMargin(0.05);
    padLegend->SetTopMargin(0.05);
    padLegend->SetBottomMargin(0.01);
    padLegend->Draw();

    // -- Main pad (linear efficiency, 0-100%) --------------------------------
    padMain->cd();

    const int n = static_cast<int>(stages_.size());
    const std::string title = ";" + x_label_ + ";" + y_label_;
    TH1F *h = new TH1F("h_surv", title.c_str(), n, 0.5, n + 0.5);
    h->SetDirectory(nullptr);

    for (int i = 0; i < n; ++i) {
      h->GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
      h->SetBinContent(i + 1, survival_.at(i) * 100.0);
    }

    h->SetMinimum(0.0);
    h->SetMaximum(100.0);

    const double xLab = gStyle->GetLabelSize("X") * 0.85;
    const double yLab = gStyle->GetLabelSize("Y") * 0.85;
    const double xTit = gStyle->GetTitleSize("X") * 0.85;
    const double yTit = gStyle->GetTitleSize("Y") * 0.85;

    h->GetXaxis()->SetLabelFont(gStyle->GetLabelFont("X"));
    h->GetYaxis()->SetLabelFont(gStyle->GetLabelFont("Y"));
    h->GetXaxis()->SetTitleFont(gStyle->GetTitleFont("X"));
    h->GetYaxis()->SetTitleFont(gStyle->GetTitleFont("Y"));
    h->GetXaxis()->SetLabelSize(xLab);
    h->GetYaxis()->SetLabelSize(yLab);
    h->GetXaxis()->SetTitleSize(xTit);
    h->GetYaxis()->SetTitleSize(yTit);
    h->GetXaxis()->SetNdivisions(510);
    h->GetYaxis()->SetNdivisions(510);
    h->GetXaxis()->SetTickLength(0.02);
    h->GetYaxis()->SetTickLength(0.02);

    h->Draw("hist");

    // Systematic band on efficiency (if provided)
    TGraphAsymmErrors *gb = nullptr;
    if (syst_low_.size() == static_cast<size_t>(n) &&
        syst_high_.size() == static_cast<size_t>(n)) {
      gb = new TGraphAsymmErrors(n);
      for (int i = 0; i < n; ++i) {
        gb->SetPoint(i, i + 1, survival_.at(i) * 100.0);
        gb->SetPointError(i, 0.0, 0.0, syst_low_.at(i) * 100.0,
                          syst_high_.at(i) * 100.0);
      }
      gb->SetFillColorAlpha(band_color_, band_alpha_);
      gb->SetLineColorAlpha(band_color_, 0.0);
      gb->Draw("2 SAME");
    }

    // Statistical error bars on efficiency (no markers)
    auto *g_err = new TGraphAsymmErrors(n);
    g_err->SetMarkerStyle(0);
    g_err->SetLineWidth(2);
    for (int i = 0; i < n; ++i) {
      g_err->SetPoint(i, i + 1, survival_.at(i) * 100.0);
      g_err->SetPointError(i, 0.0, 0.0, err_low_.at(i) * 100.0,
                          err_high_.at(i) * 100.0);
    }
    g_err->Draw("E SAME");

    // -- Overlay pad for background rejection --------------------------------
    TGraph *gp_mc = nullptr;
    TGraph *gp_tot = nullptr;

    const bool have_bkg_rej =
        (mc_bkg_rej_.size() == static_cast<size_t>(n)) ||
        (total_bkg_rej_.size() == static_cast<size_t>(n));

    TPad *padOverlay = nullptr;

    if (have_bkg_rej) {
      canvas.cd();
      padOverlay = new TPad("padOverlay", "padOverlay", 0.0, 0.0, 1.0, split);
      padOverlay->SetFillStyle(4000);      // transparent fill
      padOverlay->SetFrameFillStyle(0);    // transparent frame
      padOverlay->SetLeftMargin(padMain->GetLeftMargin());
      padOverlay->SetRightMargin(padMain->GetRightMargin());
      padOverlay->SetTopMargin(padMain->GetTopMargin());
      padOverlay->SetBottomMargin(padMain->GetBottomMargin());
      padOverlay->Draw();
      padOverlay->cd();

      auto pct = [](double f) { return f * 100.0; };

      double y2min = std::numeric_limits<double>::infinity();
      double y2max = -std::numeric_limits<double>::infinity();

      if (mc_bkg_rej_.size() == static_cast<size_t>(n)) {
        gp_mc = new TGraph(n);
        gp_mc->SetLineColor(kRed);
        gp_mc->SetMarkerColor(kRed);
        gp_mc->SetMarkerStyle(24);
        for (int i = 0; i < n; ++i) {
          const double y = pct(mc_bkg_rej_.at(i));
          gp_mc->SetPoint(i, i + 1, y);
          y2min = std::min(y2min, y);
          y2max = std::max(y2max, y);
        }
      }

      if (total_bkg_rej_.size() == static_cast<size_t>(n)) {
        gp_tot = new TGraph(n);
        gp_tot->SetLineColor(kBlue);
        gp_tot->SetMarkerColor(kBlue);
        gp_tot->SetMarkerStyle(25);
        for (int i = 0; i < n; ++i) {
          const double y = pct(total_bkg_rej_.at(i));
          gp_tot->SetPoint(i, i + 1, y);
          y2min = std::min(y2min, y);
          y2max = std::max(y2max, y);
        }
      }

      if (!std::isfinite(y2min) || !std::isfinite(y2max) || !(y2min < y2max)) {
        y2min = 0.0;
        y2max = 100.0;
      }
      // add a little headroom/footroom
      y2min = std::max(0.0, y2min - 5.0);
      y2max = std::min(100.0, y2max + 5.0);

      // Dummy frame defines overlay ranges; hide all its axes/ticks
      TH1F *h2 = padOverlay->DrawFrame(0.5, y2min, n + 0.5, y2max);
      h2->SetDirectory(nullptr);
      h2->GetXaxis()->SetLabelSize(0);
      h2->GetXaxis()->SetTickLength(0);
      h2->GetYaxis()->SetLabelSize(0);
      h2->GetYaxis()->SetTickLength(0);

      // Draw background rejection graphs
      if (gp_mc)  gp_mc->Draw("PL SAME");
      if (gp_tot) gp_tot->Draw("PL SAME");

      // Right-hand axis in overlay pad user coords
      const double xRight = n + 0.5;
      TGaxis *y2 = new TGaxis(xRight, y2min, xRight, y2max, y2min, y2max, 510);
      y2->SetTitle(y2_label_.c_str());    // e.g. "Background rejection (%)"
      y2->SetLabelFont(gStyle->GetLabelFont("Y"));
      y2->SetTitleFont(gStyle->GetTitleFont("Y"));
      y2->SetLabelSize(gStyle->GetLabelSize("Y") * 0.85);
      y2->SetTitleSize(gStyle->GetTitleSize("Y") * 0.85);
      y2->SetTitleOffset(1.2);
      y2->Draw();
    }

    // -- Legend ---------------------------------------------------------------
    canvas.cd();
    padLegend->cd();
    TLegend *legend = new TLegend(0.12, 0.0, 0.95, 0.75);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->SetTextFont(42);

    int n_entries = 1 + (gb ? 1 : 0) + 1 + (gp_mc ? 1 : 0) + (gp_tot ? 1 : 0);
    legend->SetNColumns((n_entries > 4) ? 3 : 2);
    legend->AddEntry(h, eff_label_.c_str(), "l");
    legend->AddEntry(g_err, "Stat. unc.", "le");
    if (gb)     legend->AddEntry(gb, "Syst. unc.", "f");
    if (gp_mc)  legend->AddEntry(gp_mc, "MC bkg rejection", "pl");
    if (gp_tot) legend->AddEntry(gp_tot, "Total bkg rejection", "pl");
    legend->Draw();

    canvas.cd();
  }

private:
  std::vector<std::string> stages_;
  std::vector<double> survival_;   // fractions [0..1]
  std::vector<double> err_low_;    // fractional errors [0..1]
  std::vector<double> err_high_;   // fractional errors [0..1]
  double N0_;
  std::vector<double> counts_;
  std::vector<CutFlowLossInfo> losses_;
  double pot_scale_;
  std::string x_label_;
  std::string y_label_;
  std::string eff_label_;
  std::vector<double> mc_bkg_rej_;     // fractions [0..1]
  std::vector<double> total_bkg_rej_;  // fractions [0..1]
  std::string y2_label_;
  std::vector<double> syst_low_;   // fractional syst [0..1]
  std::vector<double> syst_high_;  // fractional syst [0..1]
  int band_color_;
  double band_alpha_;
};

} // namespace analysis

#endif
