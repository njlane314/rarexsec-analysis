#ifndef SELECTION_EFFICIENCY_PLOT_H
#define SELECTION_EFFICIENCY_PLOT_H

#include <string>
#include <vector>

#include "TGraphErrors.h"
#include "TH1F.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLegendEntry.h"
#include "TPad.h"
#include "TGaxis.h"

#include "HistogramPlotterBase.h"

namespace analysis {

class SelectionEfficiencyPlot : public HistogramPlotterBase {
  public:
    SelectionEfficiencyPlot(std::string plot_name, std::vector<std::string> stages, std::vector<double> efficiencies,
                            std::vector<double> efficiency_errors, std::vector<double> purities,
                            std::vector<double> purity_errors, std::string output_directory = "plots",
                            bool log_purity = false)
        : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), stages_(std::move(stages)),
          efficiencies_(std::move(efficiencies)), efficiency_errors_(std::move(efficiency_errors)),
          purities_(std::move(purities)), purity_errors_(std::move(purity_errors)), log_purity_(log_purity) {}

  private:
    void draw(TCanvas &canvas) override {
        canvas.cd();
        int n = stages_.size();
        TPad pad1("pad1", "", 0, 0, 1, 1);
        pad1.SetRightMargin(0);
        pad1.Draw();
        pad1.cd();
        TH1F frame1("frame1", "", n, 0, n);
        frame1.GetYaxis()->SetRangeUser(0, 1.05);
        frame1.GetYaxis()->SetTitle("Efficiency");
        for (int i = 0; i < n; ++i)
            frame1.GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
        frame1.Draw("AXIS");
        TGraphErrors eff_graph(n);
        for (int i = 0; i < n; ++i) {
            double x = i + 0.5;
            eff_graph.SetPoint(i, x, efficiencies_[i]);
            eff_graph.SetPointError(i, 0.0, efficiency_errors_[i]);
        }
        eff_graph.SetLineColor(kBlue + 1);
        eff_graph.SetMarkerColor(kBlue + 1);
        eff_graph.SetMarkerStyle(20);
        eff_graph.SetLineWidth(2);
        eff_graph.Draw("PL");
        canvas.cd();
        TPad pad2("pad2", "", 0, 0, 1, 1);
        pad2.SetFillStyle(4000);
        pad2.SetFrameFillStyle(4000);
        pad2.SetLeftMargin(0);
        pad2.SetRightMargin(0.15);
        if (log_purity_)
            pad2.SetLogy();
        pad2.Draw();
        pad2.cd();
        double min_p = 1;
        for (double v : purities_)
            if (v > 0 && v < min_p)
                min_p = v;
        double y_min = log_purity_ ? (min_p > 0 ? min_p * 0.5 : 1e-3) : 0.0;
        TH1F frame2("frame2", "", n, 0, n);
        frame2.GetYaxis()->SetRangeUser(y_min, 1.05);
        frame2.GetXaxis()->SetLabelSize(0);
        frame2.GetXaxis()->SetTickLength(0);
        frame2.Draw("AXIS");
        TGraphErrors pur_graph(n);
        for (int i = 0; i < n; ++i) {
            double x = i + 0.5;
            pur_graph.SetPoint(i, x, purities_[i]);
            pur_graph.SetPointError(i, 0.0, purity_errors_[i]);
        }
        pur_graph.SetLineColor(kRed + 1);
        pur_graph.SetMarkerColor(kRed + 1);
        pur_graph.SetMarkerStyle(21);
        pur_graph.SetLineWidth(2);
        pur_graph.Draw("PL");
        TGaxis axis(n, y_min, n, 1.05, y_min, 1.05, 510, log_purity_ ? "L" : "");
        axis.SetLabelColor(kRed + 1);
        axis.SetTitleColor(kRed + 1);
        axis.SetTitle("Purity");
        axis.SetTitleOffset(1.2);
        axis.Draw();
        canvas.cd();
        TLegend legend(0.6, 0.75, 0.88, 0.88);
        legend.SetBorderSize(0);
        legend.SetFillStyle(0);
        legend.SetTextFont(42);
        auto *eff_entry = legend.AddEntry((TObject *)nullptr, "Signal Efficiency", "lep");
        eff_entry->SetLineColor(kBlue + 1);
        eff_entry->SetMarkerColor(kBlue + 1);
        eff_entry->SetMarkerStyle(20);
        auto *pur_entry = legend.AddEntry((TObject *)nullptr, "Signal Purity", "lep");
        pur_entry->SetLineColor(kRed + 1);
        pur_entry->SetMarkerColor(kRed + 1);
        pur_entry->SetMarkerStyle(21);
        legend.Draw();
        TLatex latex;
        latex.SetTextAlign(23);
        latex.SetTextFont(42);
        latex.SetTextSize(0.035);
        for (int i = 0; i < n; ++i) {
            double x = i + 0.5;
            double ye = efficiencies_[i];
            double yp = purities_[i];
            latex.SetTextColor(kBlue + 1);
            latex.DrawLatex(x, ye + 0.02, Form("%.2f", ye));
            latex.SetTextColor(kRed + 1);
            double yp_text = log_purity_ ? yp * 1.1 : yp + 0.02;
            latex.DrawLatex(x, yp_text, Form("%.2f", yp));
        }
    }

    std::vector<std::string> stages_;
    std::vector<double> efficiencies_;
    std::vector<double> efficiency_errors_;
    std::vector<double> purities_;
    std::vector<double> purity_errors_;
    bool log_purity_;
};

} // namespace analysis

#endif
