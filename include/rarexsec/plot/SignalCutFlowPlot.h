#ifndef SIGNALCUTFLOWPLOT_H
#define SIGNALCUTFLOWPLOT_H

#include <string>
#include <vector>

#include "TGraphAsymmErrors.h"
#include "TH1F.h"
#include "TLatex.h"
#include "TString.h"

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
                    std::string x_label = "Cut Stage",
                    std::string y_label = "Survival Probability (%)")
      : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
        stages_(std::move(stages)), survival_(std::move(survival)),
        err_low_(std::move(err_low)), err_high_(std::move(err_high)), N0_(N0),
        counts_(std::move(counts)), losses_(std::move(losses)),
        pot_scale_(pot_scale), x_label_(std::move(x_label)),
        y_label_(std::move(y_label)) {}

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
    h->SetMinimum(0.0);
    h->SetMaximum(100.0);
    h->GetXaxis()->SetTitleFont(42);
    h->GetYaxis()->SetTitleFont(42);
    h->GetXaxis()->SetLabelFont(42);
    h->GetYaxis()->SetLabelFont(42);
    h->GetXaxis()->SetTitleOffset(1.0);
    h->GetYaxis()->SetTitleOffset(1.0);
    h->Draw("hist");

    auto *g = new TGraphAsymmErrors(n);
    for (int i = 0; i < n; ++i) {
      g->SetPoint(i, i + 1, survival_[i] * 100.0);
      g->SetPointError(i, 0.0, 0.0, err_low_[i] * 100.0, err_high_[i] * 100.0);
    }
    g->Draw("P SAME");

    TLatex latex;
    latex.SetTextAlign(21);
    latex.SetTextFont(h->GetXaxis()->GetTitleFont());
    latex.SetTextSize(h->GetXaxis()->GetLabelSize());
    for (int i = 0; i < n; ++i) {
      auto txt = TString::Format("%0.1f/%0.1f", counts_[i] * pot_scale_,
                                 N0_ * pot_scale_);
      latex.DrawLatex(i + 1, survival_[i] * 100.0 + 3.0, txt.Data());
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
        auto txt = TString::Format("-%0.1f%%: %s (%0.0f%%)", drop,
                                   info.reason.c_str(), frac * 100.0);
        latex.DrawLatex(i + 1 - 0.1, survival_[i] * 100.0 + 7.0, txt.Data());
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
};

} // namespace analysis

#endif
