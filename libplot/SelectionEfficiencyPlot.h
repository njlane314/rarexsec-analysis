#ifndef SELECTION_EFFICIENCY_PLOT_H
#define SELECTION_EFFICIENCY_PLOT_H

#include <string>
#include <vector>

#include "TGraphErrors.h"
#include "TH1F.h"
#include "TLegend.h"

#include "HistogramPlotterBase.h"

namespace analysis {

class SelectionEfficiencyPlot : public HistogramPlotterBase {
public:
    SelectionEfficiencyPlot(std::string plot_name,
                            std::vector<std::string> stages,
                            std::vector<double> efficiencies,
                            std::vector<double> efficiency_errors,
                            std::vector<double> purities,
                            std::vector<double> purity_errors,
                            std::string output_directory = "plots")
      : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)),
        stages_(std::move(stages)),
        efficiencies_(std::move(efficiencies)),
        efficiency_errors_(std::move(efficiency_errors)),
        purities_(std::move(purities)),
        purity_errors_(std::move(purity_errors)) {}

private:
    void draw(TCanvas& canvas) override {
        canvas.cd();
        int n = stages_.size();
        TH1F frame("frame", "", n, 0, n);
        frame.GetYaxis()->SetRangeUser(0.0, 1.05);
        frame.GetYaxis()->SetTitle("Fraction");
        for (int i = 0; i < n; ++i) {
            frame.GetXaxis()->SetBinLabel(i + 1, stages_[i].c_str());
        }
        frame.Draw("AXIS");

        TGraphErrors eff_graph(n);
        TGraphErrors pur_graph(n);
        for (int i = 0; i < n; ++i) {
            double x = i + 0.5;
            eff_graph.SetPoint(i, x, efficiencies_[i]);
            eff_graph.SetPointError(i, 0.0, efficiency_errors_[i]);
            pur_graph.SetPoint(i, x, purities_[i]);
            pur_graph.SetPointError(i, 0.0, purity_errors_[i]);
        }
        eff_graph.SetLineColor(kBlue+1);
        eff_graph.SetMarkerColor(kBlue+1);
        eff_graph.SetMarkerStyle(20);
        pur_graph.SetLineColor(kRed+1);
        pur_graph.SetMarkerColor(kRed+1);
        pur_graph.SetMarkerStyle(21);

        eff_graph.Draw("P SAME");
        pur_graph.Draw("P SAME");

        TLegend legend(0.6, 0.75, 0.88, 0.88);
        legend.SetBorderSize(0);
        legend.SetFillStyle(0);
        legend.SetTextFont(42);
        legend.AddEntry(&eff_graph, "Signal Efficiency", "lep");
        legend.AddEntry(&pur_graph, "Signal Purity", "lep");
        legend.Draw();
    }

    std::vector<std::string> stages_;
    std::vector<double>       efficiencies_;
    std::vector<double>       efficiency_errors_;
    std::vector<double>       purities_;
    std::vector<double>       purity_errors_;
};

}

#endif
