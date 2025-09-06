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
                 std::string output_directory = "plots");

  private:
    void draw(TCanvas &canvas) override;

    std::vector<double> signal_eff_;
    std::vector<double> background_rej_;
};

}

#endif
