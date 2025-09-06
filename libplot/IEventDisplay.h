#ifndef IEVENTDISPLAY_H
#define IEVENTDISPLAY_H

#include "AnalysisLogger.h"
#include "TCanvas.h"
#include "TColor.h"
#include "TH2F.h"
#include "TStyle.h"
#include "TSystem.h"
#include <string>
#include <vector>

namespace analysis {

class IEventDisplay {
  public:
    IEventDisplay(std::string tag, int image_size, std::string output_directory)
        : tag_(std::move(tag)), image_size_(image_size), output_directory_(std::move(output_directory)) {}

    virtual ~IEventDisplay() = default;

    void drawAndSave(const std::string &format = "png") {
        gSystem->mkdir(output_directory_.c_str(), true);

        log::info("EventDisplay", "Saving", tag_, "to", output_directory_);

        TCanvas canvas(tag_.c_str(), tag_.c_str(), image_size_, image_size_);
        draw(canvas);
        canvas.SaveAs((output_directory_ + "/" + tag_ + "." + format).c_str());
    }

  protected:
    virtual void draw(TCanvas &canvas) = 0;

    std::string tag_;
    int image_size_;
    std::string output_directory_;
};

class DetectorDisplay : public IEventDisplay {
  public:
    DetectorDisplay(std::string tag, std::vector<float> data, int image_size, std::string output_directory)
        : IEventDisplay(std::move(tag), image_size, std::move(output_directory)), data_(std::move(data)) {}

  protected:
    void draw(TCanvas &canvas) override {
        const int bin_offset = 1;
        const float threshold = 4;
        const float min_val = 1;
        const float max_val = 1000;

        TH2F hist(tag_.c_str(), tag_.c_str(), image_size_, 0, image_size_, image_size_, 0, image_size_);

        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                float v = data_[r * image_size_ + c];
                hist.SetBinContent(c + bin_offset, r + bin_offset, v > threshold ? v : min_val);
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

class SemanticDisplay : public IEventDisplay {
  public:
    SemanticDisplay(std::string tag, std::vector<int> data, int image_size, std::string output_directory)
        : IEventDisplay(std::move(tag), image_size, std::move(output_directory)), data_(std::move(data)) {}

  protected:
    void draw(TCanvas &) override {
        const int palette_size = 10;
        const int palette_step = 2;
        const int bin_offset = 1;
        const int stats_off = 0;
        const double z_min = -0.5;
        const double z_max = 9.5;

        TH2F hist(tag_.c_str(), tag_.c_str(), image_size_, 0, image_size_, image_size_, 0, image_size_);

        int palette[palette_size];
        for (int i = 0; i < palette_size; ++i)
            palette[i] = kWhite + (i > 0 ? i * palette_step : 0);
        gStyle->SetPalette(palette_size, palette);

        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                hist.SetBinContent(c + bin_offset, r + bin_offset, data_[r * image_size_ + c]);
            }
        }

        hist.SetStats(stats_off);
        hist.GetZaxis()->SetRangeUser(z_min, z_max);
        hist.GetXaxis()->SetTitle("Time");
        hist.GetYaxis()->SetTitle("Wire");
        hist.Draw("COL");
    }

  private:
    std::vector<int> data_;
};

}

#endif
