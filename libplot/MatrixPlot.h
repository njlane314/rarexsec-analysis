#ifndef MATRIXPLOT_H
#define MATRIXPLOT_H

#include <string>

#include "TCanvas.h"
#include "TH2F.h"
#include "TStyle.h"

#include "IHistogramPlot.h"

namespace analysis {

class MatrixPlot : public IHistogramPlot {
  public:
    MatrixPlot(std::string plot_name, TH2F *hist, std::string output_directory = "plots")
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)), hist_(hist) {}

    ~MatrixPlot() override { delete hist_; }

  private:
    void draw(TCanvas &canvas) override {
        canvas.cd();
        const int stats_off = 0;
        const int contour_count = 255;
        const double margin = 0.15;
        const double title_offset = 1.2;

        gStyle->SetOptStat(stats_off);
        gStyle->SetNumberContours(contour_count);

        canvas.SetLogz();
        canvas.SetLeftMargin(margin);
        canvas.SetRightMargin(margin);

        hist_->SetTitle("");
        hist_->GetZaxis()->SetTitleOffset(title_offset);
        hist_->Draw("COLZ");
    }

    TH2F *hist_;
};

}

#endif
