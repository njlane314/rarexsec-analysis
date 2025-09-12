#ifndef DETECTORDISPLAY_H
#define DETECTORDISPLAY_H

#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TH2F.h"

#include <rarexsec/plot/IEventDisplay.h>

namespace analysis {

class DetectorDisplay : public IEventDisplay {
public:
  DetectorDisplay(std::string tag, std::string title, std::vector<float> data,
                  int canvas_size, std::string output_directory)
      : IEventDisplay(std::move(tag), std::move(title), canvas_size,
                      std::move(output_directory)),
        data_(std::move(data)) {}

protected:
  void draw(TCanvas &canvas) override {
    const int bin_offset = 1;
    const float threshold = 4;
    const float min_val = 1;
    const float max_val = 1000;

    int dim = static_cast<int>(std::sqrt(data_.size()));
    hist_ = std::make_unique<TH2F>(tag_.c_str(), title_.c_str(), dim, 0, dim,
                                   dim, 0, dim);

    for (int r = 0; r < dim; ++r) {
      for (int c = 0; c < dim; ++c) {
        float v = data_[r * dim + c];
        hist_->SetBinContent(c + bin_offset, r + bin_offset,
                             v > threshold ? v : min_val);
      }
    }

    canvas.SetLogz();
    canvas.SetTicks(0, 0);
    hist_->SetStats(false);
    hist_->SetMinimum(min_val);
    hist_->SetMaximum(max_val);
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
  std::vector<float> data_;
  std::unique_ptr<TH2F> hist_;
};

} // namespace analysis

#endif
