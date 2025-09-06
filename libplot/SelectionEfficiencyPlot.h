#ifndef SELECTION_EFFICIENCY_PLOT_H
#define SELECTION_EFFICIENCY_PLOT_H

#include <string>
#include <vector>
#include <utility>

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
                            bool use_log_y = false);

  private:
    void draw(TCanvas &canvas) override;

    void setupFrame(TCanvas &canvas, TH1F &frame);

    std::pair<TGraphErrors, TGraphErrors> buildGraphs();

    void annotatePoints();

    TLegend buildLegend();

    std::vector<std::string> stages_;
    std::vector<double> efficiencies_;
    std::vector<double> efficiency_errors_;
    std::vector<double> purities_;
    std::vector<double> purity_errors_;
    bool use_log_y_;
};

}

#endif
