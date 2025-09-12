#ifndef DETECTORDISPLAY_H
#define DETECTORDISPLAY_H

#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TH2F.h"

#include <rarexsec/plot/IEventDisplay.h>

namespace analysis {

class DetectorDisplay : public IEventDisplay {
  public:
    DetectorDisplay(std::string tag, std::vector<float> data, int image_size,
                    std::string output_directory)
        : IEventDisplay(std::move(tag), image_size,
                        std::move(output_directory)),
          data_(std::move(data)) {}

  protected:
    void draw(TCanvas &canvas) override {
        const int bin_offset = 1;
        const float threshold = 4;
        const float min_val = 1;
        const float max_val = 1000;

        hist_ = std::make_unique<TH2F>(tag_.c_str(), tag_.c_str(), image_size_, 0,
                                       image_size_, image_size_, 0, image_size_);

        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                float v = data_[r * image_size_ + c];
                hist_->SetBinContent(c + bin_offset, r + bin_offset,
                                     v > threshold ? v : min_val);
            }
        }

        canvas.SetLogz();
        hist_->SetMinimum(min_val);
        hist_->SetMaximum(max_val);
        hist_->GetXaxis()->SetTitle("Wire");
        hist_->GetYaxis()->SetTitle("Time");
        hist_->Draw("COL");
    }

private:
    std::vector<float> data_;
    std::unique_ptr<TH2F> hist_;
};

} 

#endif
