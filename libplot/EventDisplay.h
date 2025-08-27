#ifndef EVENT_DISPLAY_H
#define EVENT_DISPLAY_H

#include "AnalysisLogger.h"
#include "HistogramPlotterBase.h"
#include "TCanvas.h"
#include "TH2F.h"
#include "TStyle.h"
#include <string>
#include <vector>

namespace analysis {

class EventDisplayBase : public HistogramPlotterBase {
  public:
    EventDisplayBase(std::string plot_name, int image_size, std::string output_directory)
        : HistogramPlotterBase(std::move(plot_name), std::move(output_directory)), image_size_(image_size) {}

    void drawAndSave(const std::string &format = "png") {
        log::info("EventDisplay", "Saving", plot_name_, "to", output_directory_);
        HistogramPlotterBase::drawAndSave(format);
    }

  protected:
    int image_size_;
};

class DetectorDisplayPlot : public EventDisplayBase {
  public:
    DetectorDisplayPlot(std::string tag, std::vector<float> data, int image_size, std::string output_directory)
        : EventDisplayBase(std::move(tag), image_size, std::move(output_directory)), data_(std::move(data)) {}

    void draw(TCanvas &canvas) override {
        TH2F hist(plot_name_.c_str(), plot_name_.c_str(), image_size_, 0, image_size_, image_size_, 0, image_size_);
        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                float v = data_[r * image_size_ + c];
                hist.SetBinContent(c + 1, r + 1, v > 4 ? v : 1);
            }
        }
        canvas.SetLogz();
        hist.SetMinimum(1);
        hist.SetMaximum(1000);
        hist.GetXaxis()->SetTitle("Wire");
        hist.GetYaxis()->SetTitle("Time");
        hist.Draw("COL");
    }

  private:
    std::vector<float> data_;
};

class SemanticDisplayPlot : public EventDisplayBase {
  public:
    SemanticDisplayPlot(std::string tag, std::vector<int> data, int image_size, std::string output_directory)
        : EventDisplayBase(std::move(tag), image_size, std::move(output_directory)), data_(std::move(data)) {}

    void draw(TCanvas &canvas) override {
        canvas.cd();
        TH2F hist(plot_name_.c_str(), plot_name_.c_str(), image_size_, 0, image_size_, image_size_, 0, image_size_);
        int palette[10];
        for (int i = 0; i < 10; ++i)
            palette[i] = kWhite + (i > 0 ? i * 2 : 0);
        gStyle->SetPalette(10, palette);
        for (int r = 0; r < image_size_; ++r) {
            for (int c = 0; c < image_size_; ++c) {
                hist.SetBinContent(c + 1, r + 1, data_[r * image_size_ + c]);
            }
        }
        hist.SetStats(0);
        hist.GetZaxis()->SetRangeUser(-0.5, 9.5);
        hist.GetXaxis()->SetTitle("Time");
        hist.GetYaxis()->SetTitle("Wire");
        hist.Draw("COL");
    }

  private:
    std::vector<int> data_;
};

}

#endif
