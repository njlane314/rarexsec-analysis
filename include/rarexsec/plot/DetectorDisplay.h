#ifndef DETECTORDISPLAY_H
#define DETECTORDISPLAY_H

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

        TH2F hist(tag_.c_str(), tag_.c_str(), image_size_, 0, image_size_,
                  image_size_, 0, image_size_);

        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                float v = data_[r * image_size_ + c];
                hist.SetBinContent(c + bin_offset, r + bin_offset,
                                   v > threshold ? v : min_val);
            }
        }

        canvas.SetLogz();
        hist.SetMinimum(min_val);
        hist.SetMaximum(max_val);
        hist.GetXaxis()->SetTitle("Wire");
        hist.GetYaxis()->SetTitle("Time");
        hist.Draw("COL");
    }

private:
    std::vector<float> data_;
};

} 

#endif
