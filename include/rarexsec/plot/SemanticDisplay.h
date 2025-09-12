#ifndef SEMANTICDISPLAY_H
#define SEMANTICDISPLAY_H

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TColor.h"
#include "TH2F.h"
#include "TStyle.h"

#include <rarexsec/plot/IEventDisplay.h>

namespace analysis {

class SemanticDisplay : public IEventDisplay {
public:
  SemanticDisplay(std::string tag, std::string title, std::vector<int> data,
                  int canvas_size, std::string output_directory)
      : IEventDisplay(std::move(tag), std::move(title), canvas_size,
                      std::move(output_directory)),
        data_(std::move(data)) {}

protected:
  void draw(TCanvas &canvas) override {
    const int palette_size = 10;
    const int palette_step = 2;
    const int bin_offset = 1;
    const int stats_off = 0;
    const double z_min = -0.5;
    const double z_max = 9.5;

    int dim = static_cast<int>(std::sqrt(data_.size()));
    hist_ = std::make_unique<TH2F>(tag_.c_str(), title_.c_str(), dim, 0, dim,
                                   dim, 0, dim);

    int palette[palette_size];
    for (int i = 0; i < palette_size; ++i)
      palette[i] = kWhite + (i > 0 ? i * palette_step : 0);
    gStyle->SetPalette(palette_size, palette);

    for (int r = 0; r < dim; ++r) {
      for (int c = 0; c < dim; ++c) {
        hist_->SetBinContent(c + bin_offset, r + bin_offset,
                             data_[r * dim + c]);
      }
    }

    hist_->SetStats(stats_off);
    hist_->GetZaxis()->SetRangeUser(z_min, z_max);
    canvas.SetTicks(0, 0);
    hist_->GetXaxis()->SetTitle("Local Wire Coordinate");
    hist_->GetYaxis()->SetTitle("Local Drift Coordinate");
    hist_->GetXaxis()->CenterTitle(true);
    hist_->GetYaxis()->CenterTitle(true);
    hist_->GetXaxis()->SetTickLength(0);
    hist_->GetYaxis()->SetTickLength(0);
    hist_->GetXaxis()->SetLabelSize(0);
    hist_->GetYaxis()->SetLabelSize(0);
    hist_->GetXaxis()->SetAxisColor(0);
    hist_->GetYaxis()->SetAxisColor(0);
    hist_->Draw("COL");
  }

private:
  std::vector<int> data_;
  std::unique_ptr<TH2F> hist_;
};

} // namespace analysis

#endif
