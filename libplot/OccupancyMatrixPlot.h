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
    OccupancyMatrixPlot(std::string plot_name, TH2F *hist,
                        std::string output_directory = "plots")
        : HistogramPlotterBase(std::move(plot_name),
                               std::move(output_directory)),
          hist_(hist) {}

    ~OccupancyMatrixPlot() override { delete hist_; }

  private:
    void draw(TCanvas &canvas) override {
        canvas.cd();

        gStyle->SetOptStat(0);
        gStyle->SetNumberContours(255);

        canvas.SetLogz();
        canvas.SetLeftMargin(0.15);
        canvas.SetRightMargin(0.15);

        hist_->SetTitle("");
        hist_->GetZaxis()->SetTitleOffset(1.2);

        hist_->Draw("COLZ");
    }

    TH2F *hist_;
};

}

#endif
