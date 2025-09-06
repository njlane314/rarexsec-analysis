#ifndef OCCUPANCY_MATRIX_PLOT_H
#define OCCUPANCY_MATRIX_PLOT_H

#include <string>

#include "TCanvas.h"
#include "TH2F.h"
#include "TStyle.h"

#include "HistogramPlotterBase.h"

namespace analysis {

class OccupancyMatrixPlot : public HistogramPlotterBase {
  public:
    OccupancyMatrixPlot(std::string plot_name, TH2F *hist, std::string output_directory = "plots");

    ~OccupancyMatrixPlot() override;

  private:
    void draw(TCanvas &canvas) override;

    TH2F *hist_;
};

}

#endif
