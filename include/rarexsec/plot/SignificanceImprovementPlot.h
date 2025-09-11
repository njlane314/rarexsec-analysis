#ifndef SIGNIFICANCEIMPROVEMENTPLOT_H
#define SIGNIFICANCEIMPROVEMENTPLOT_H

#include <string>
#include <vector>
#include <algorithm>

#include "TCanvas.h"
#include "TGraph.h"

#include <rarexsec/plot/IHistogramPlot.h>

namespace analysis {

class SignificanceImprovementPlot : public IHistogramPlot {
  public:
    SignificanceImprovementPlot(std::string plot_name, std::vector<double> signal_eff,
                                std::vector<double> significance_improvement,
                                std::string output_directory = "plots")
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)),
          signal_eff_(std::move(signal_eff)),
          significance_improvement_(std::move(significance_improvement)) {}

  protected:
    void draw(TCanvas &canvas) override {
        canvas.cd();
        int n = signal_eff_.size();
        TGraph graph(n);
        for (int i = 0; i < n; ++i)
            graph.SetPoint(i, signal_eff_[i], significance_improvement_[i]);

        const int colour_offset = 1;
        const int line_width = 2;
        const int marker_style = 20;
        const double axis_min = 0.0;

        graph.SetLineColor(kBlue + colour_offset);
        graph.SetLineWidth(line_width);
        graph.SetMarkerColor(kBlue + colour_offset);
        graph.SetMarkerStyle(marker_style);
        graph.GetXaxis()->SetTitle("Signal Efficiency");
        graph.GetYaxis()->SetTitle("S/#sqrt{B} (relative)");
        graph.GetXaxis()->SetLimits(axis_min, 1.0);
        double max_val = *std::max_element(significance_improvement_.begin(),
                                           significance_improvement_.end());
        graph.GetYaxis()->SetRangeUser(axis_min, max_val * 1.05);
        graph.DrawClone("ALP");
    }

  private:
    std::vector<double> signal_eff_;
    std::vector<double> significance_improvement_;
};

} // namespace analysis

#endif
