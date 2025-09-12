#ifndef SEMANTICDISPLAY_H
#define SEMANTICDISPLAY_H

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
                  int image_size, std::string output_directory)
      : IEventDisplay(std::move(tag), std::move(title), image_size,
                      std::move(output_directory)),
        data_(std::move(data)) {}

protected:
  void draw(TCanvas &canvas) override {
    constexpr int palette_size = 15;
    const int palette[palette_size] = {
        kGray,      // Empty
        kGray + 1,  // Cosmic
        kBlue,      // Muon
        kMagenta,   // Electron
        kCyan,      // Photon
        kRed,       // ChargedPion
        kOrange,    // NeutralPion
        kYellow,    // Neutron
        kGreen,     // Proton
        kTeal,      // ChargedKaon
        kAzure,     // NeutralKaon
        kViolet,    // Lambda
        kPink,      // ChargedSigma
        kSpring,    // NeutralSigma
        kBlack      // Other
    };
    constexpr int bin_offset = 1;
    constexpr int stats_off = 0;
    constexpr double z_min = -0.5;
    constexpr double z_max = palette_size - 0.5;

    hist_ = std::make_unique<TH2F>(tag_.c_str(), title_.c_str(), image_size_, 0,
                                   image_size_, image_size_, 0, image_size_);

    gStyle->SetPalette(palette_size, palette);

    for (int r = 0; r < image_size_; ++r) {
      for (int c = 0; c < image_size_; ++c) {
        hist_->SetBinContent(c + bin_offset, r + bin_offset,
                             data_[r * image_size_ + c]);
      }
    }

    hist_->SetStats(stats_off);
    hist_->GetZaxis()->SetRangeUser(z_min, z_max);
    canvas.SetFillColor(kGray);
    canvas.SetFrameFillColor(kGray);
    canvas.SetTicks(0, 0);
    hist_->GetXaxis()->SetTitle("Local Wire Coordinate");
    hist_->GetYaxis()->SetTitle("Local Drift Coordinate");
    hist_->GetXaxis()->CenterTitle(true);
    hist_->GetYaxis()->CenterTitle(true);
    hist_->GetXaxis()->SetTickLength(0);
    hist_->GetYaxis()->SetTickLength(0);
    hist_->GetXaxis()->SetLabelSize(0);
    hist_->GetYaxis()->SetLabelSize(0);
    hist_->Draw("COL");
  }

private:
  std::vector<int> data_;
  std::unique_ptr<TH2F> hist_;
};

} // namespace analysis

#endif
