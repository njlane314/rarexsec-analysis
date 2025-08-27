#ifndef SELECTION_EFFICIENCY_PLOT_H
#define SELECTION_EFFICIENCY_PLOT_H

#include <string>
#include <vector>

#include "TGraphErrors.h"
#include "TH1F.h"
#include "TLatex.h"
#include "TLegend.h"
#include "TLegendEntry.h"

#include "HistogramPlotterBase.h"

namespace analysis {

class SelectionEfficiencyPlot : public HistogramPlotterBase {
  public:
    SelectionEfficiencyPlot(std::string plot_name, std::vector<std::string> stages, std::vector<double> efficiencies,
                            std::vector<double> efficiency_errors, std::vector<double> purities,
                            std::vector<double> purity_errors, std::string output_directory = "plots",
                            bool use_log_y = false)
        : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), stages_(std::move(stages)),
          efficiencies_(std::move(efficiencies)), efficiency_errors_(std::move(efficiency_errors)),
          purities_(std::move(purities)), purity_errors_(std::move(purity_errors)), use_log_y_(use_log_y) {}

  private:
    void draw(TCanvas &canvas) override {
        canvas.cd();
        if (use_log_y_)
            canvas.SetLogy();
        int n = stages_.size();
        TH1F frame("frame", "", n, 0, n);
        double y_min = use_log_y_ ? 1e-3 : 0.0;
        frame.GetYaxis()->SetRangeUser(y_min, 1.05);
        frame.GetYaxis()->SetTitle("Fraction");
        for (int i = 0; i < n; ++i) {
            frame.GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
        }
        frame.DrawClone("AXIS");

        TGraphErrors eff_graph(n);
        TGraphErrors pur_graph(n);
        for (int i = 0; i < n; ++i) {
            double x = i + 0.5;
            eff_graph.SetPoint(i, x, efficiencies_[i]);
            eff_graph.SetPointError(i, 0.0, efficiency_errors_[i]);
            pur_graph.SetPoint(i, x, purities_[i]);
            pur_graph.SetPointError(i, 0.0, purity_errors_[i]);
        }
        eff_graph.SetLineColor(kBlue + 1);
        eff_graph.SetMarkerColor(kBlue + 1);
        eff_graph.SetMarkerStyle(20);
        eff_graph.SetLineWidth(2);
        pur_graph.SetLineColor(kRed + 1);
        pur_graph.SetMarkerColor(kRed + 1);
        pur_graph.SetMarkerStyle(21);
        pur_graph.SetLineWidth(2);

        eff_graph.DrawClone("PL SAME");
        pur_graph.DrawClone("PL SAME");

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
            latex.DrawLatex(x, yp + 0.02, Form("%.2f", yp));
        }

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
        legend.DrawClone();
    }

    std::vector<std::string> stages_;
    std::vector<double> efficiencies_;
    std::vector<double> efficiency_errors_;
    std::vector<double> purities_;
    std::vector<double> purity_errors_;
    bool use_log_y_;
};

} // namespace analysis

#endif
