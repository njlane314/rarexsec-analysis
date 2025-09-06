#include "OccupancyMatrixPlot.h"

namespace analysis {

OccupancyMatrixPlot::OccupancyMatrixPlot(std::string plot_name, TH2F *hist, std::string output_directory)
    : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), hist_(hist) {}

OccupancyMatrixPlot::~OccupancyMatrixPlot() { delete hist_; }

void OccupancyMatrixPlot::draw(TCanvas &canvas) {
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

}
