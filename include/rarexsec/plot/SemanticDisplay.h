#ifndef SEMANTICDISPLAY_H
#define SEMANTICDISPLAY_H

#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

#include "TCanvas.h"
#include "TColor.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TLegend.h"
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
    const int palette_size = 15;
    const int bin_offset = 1;
    const int stats_off = 0;
    const double z_min = -0.5;
    const double z_max = palette_size - 0.5;

    int dim = static_cast<int>(std::sqrt(data_.size()));
    hist_ = std::make_unique<TH2F>(tag_.c_str(), title_.c_str(), dim, 0, dim,
                                   dim, 0, dim);

    const int background = TColor::GetColor(200, 200, 200);
    std::array<int, palette_size> palette = {
        background, kBlack,     kRed,      kBlue,       kMagenta,
        kGreen + 2, kYellow,    kCyan,     kOrange + 7, kSpring + 4,
        kTeal + 3,  kAzure + 5, kPink + 5, kViolet + 5, kGray + 1};
    gStyle->SetPalette(palette_size, palette.data());
    canvas.SetFillColor(kWhite);
    canvas.SetFrameFillColor(background);

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
    // Double the axis title offset to separate the labels from the axes and
    // make them easier to read in the rendered event displays.
    constexpr double axis_offset = 0.8;
    hist_->GetXaxis()->SetTitleOffset(axis_offset);
    hist_->GetYaxis()->SetTitleOffset(axis_offset);
    hist_->GetXaxis()->SetTickLength(0);
    hist_->GetYaxis()->SetTickLength(0);
    hist_->GetXaxis()->SetLabelSize(0);
    hist_->GetYaxis()->SetLabelSize(0);
    hist_->GetXaxis()->SetAxisColor(0);
    hist_->GetYaxis()->SetAxisColor(0);
    hist_->Draw("COL");

    // Legend describing semantic classes
    legend_entries_.clear();
    legend_ = std::make_unique<TLegend>(0.80, 0.10, 0.98, 0.90);
    legend_->SetFillColor(kWhite);
    legend_->SetBorderSize(0);
    legend_->SetTextSize(0.03);

    const std::array<const char *, palette_size> labels = {
        "Empty",       "Cosmic",       "Muon",         "Electron",
        "Photon",      "ChargedPion",  "NeutralPion",  "Neutron",
        "Proton",      "ChargedKaon",  "NeutralKaon",  "Lambda",
        "ChargedSigma", "NeutralSigma", "Other"};

    for (int i = 0; i < palette_size; ++i) {
      auto h = std::make_unique<TH1F>((tag_ + std::to_string(i)).c_str(), "", 1,
                                      0, 1);
      h->SetFillColor(palette[i]);
      h->SetLineColor(palette[i]);
      legend_->AddEntry(h.get(), labels[i], "f");
      legend_entries_.push_back(std::move(h));
    }
    legend_->Draw();
  }

private:
  std::vector<int> data_;
  std::unique_ptr<TH2F> hist_;
  std::unique_ptr<TLegend> legend_;
  std::vector<std::unique_ptr<TH1F>> legend_entries_;
};

} // namespace analysis

#endif
