#ifndef SIGNALCUTFLOWPLOT_H
#define SIGNALCUTFLOWPLOT_H

#include <string>
#include <vector>

#include "TH1F.h"
#include "TGraphAsymmErrors.h"
#include "TLatex.h"
#include "TString.h"

#include <rarexsec/plot/IHistogramPlot.h>

namespace analysis {

struct CutFlowLossInfo {
    std::string reason;
    int top_count{0};
    int total{0};
};

class SignalCutFlowPlot : public IHistogramPlot {
  public:
    SignalCutFlowPlot(std::string plot_name, std::vector<std::string> stages,
                      std::vector<double> survival,
                      std::vector<double> err_low,
                      std::vector<double> err_high,
                      size_t N0,
                      std::vector<size_t> counts,
                      std::vector<CutFlowLossInfo> losses,
                      std::string output_directory = "plots")
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
          stages_(std::move(stages)), survival_(std::move(survival)),
          err_low_(std::move(err_low)), err_high_(std::move(err_high)), N0_(N0),
          counts_(std::move(counts)), losses_(std::move(losses)) {}

  protected:
    void draw(TCanvas &canvas) override {
        int n = static_cast<int>(stages_.size());
        TH1F h("h_surv", "Truth-signal cut-flow;Stage;Cumulative survival [%]",
               n, 0.5, n + 0.5);
        for (int i = 0; i < n; ++i) {
            h.GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
            h.SetBinContent(i + 1, survival_[i] * 100.0);
        }
        h.SetMinimum(0.0);
        h.SetMaximum(100.0);
        h.Draw("hist");

        TGraphAsymmErrors g(n);
        for (int i = 0; i < n; ++i) {
            g.SetPoint(i, i + 1, survival_[i] * 100.0);
            g.SetPointError(i, 0.0, 0.0, err_low_[i] * 100.0,
                            err_high_[i] * 100.0);
        }
        g.Draw("P SAME");

        TLatex latex;
        latex.SetTextAlign(21);
        latex.SetTextFont(42);
        latex.SetTextSize(0.035);
        for (int i = 0; i < n; ++i) {
            std::string txt = std::to_string(counts_[i]) + "/" + std::to_string(N0_);
            latex.DrawLatex(i + 1, survival_[i] * 100.0 + 1.5, txt.c_str());
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
                latex.DrawLatex(i + 1 - 0.05, survival_[i] * 100.0 + 4.0,
                                txt.Data());
            }
        }
    }

  private:
    std::vector<std::string> stages_;
    std::vector<double> survival_;
    std::vector<double> err_low_;
    std::vector<double> err_high_;
    size_t N0_;
    std::vector<size_t> counts_;
    std::vector<CutFlowLossInfo> losses_;
};

} // namespace analysis

#endif
