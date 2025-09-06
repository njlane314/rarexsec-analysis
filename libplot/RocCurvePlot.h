#ifndef ROC_CURVE_PLOT_H
#define ROC_CURVE_PLOT_H

#include <string>
#include <vector>

#include "TCanvas.h"
#include "TGraph.h"

#include "HistogramPlotterBase.h"

namespace analysis {

class RocCurvePlot : public HistogramPlotterBase {
  public:
    RocCurvePlot(std::string plot_name, std::vector<double> signal_eff, std::vector<double> background_rej,
                 std::string output_directory = "plots")
        : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), signal_eff_(std::move(signal_eff)),
          background_rej_(std::move(background_rej)) {}

  private:
    void draw(TCanvas &canvas) override {
        canvas.cd();
        int n = signal_eff_.size();
        TGraph graph(n);
        for (int i = 0; i < n; ++i) {
            graph.SetPoint(i, signal_eff_[i], background_rej_[i]);
        }

        const int colour_offset = 1;
        const int line_width = 2;
        const int marker_style = 20;
        const double axis_min = 0.0;
        const double axis_max = 1.0;

        graph.SetLineColor(kBlue + colour_offset);
        graph.SetLineWidth(line_width);
        graph.SetMarkerColor(kBlue + colour_offset);
        graph.SetMarkerStyle(marker_style);
        graph.GetXaxis()->SetTitle("Signal Efficiency");
        graph.GetYaxis()->SetTitle("Background Rejection");
        graph.GetXaxis()->SetLimits(axis_min, axis_max);
        graph.GetYaxis()->SetRangeUser(axis_min, axis_max);
        graph.DrawClone("ALP");
    }

    std::vector<double> signal_eff_;
    std::vector<double> background_rej_;
};

}

#endif
