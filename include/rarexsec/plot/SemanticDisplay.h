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

    // Lighter grey background for the display
    const int background = TColor::GetColor(230, 230, 230);

    // High-contrast colour palette for particle classes
    std::array<int, palette_size> palette = {
        background,
        TColor::GetColor("#000000"), // Cosmic
        TColor::GetColor("#e41a1c"), // Muon
        TColor::GetColor("#377eb8"), // Electron
        TColor::GetColor("#4daf4a"), // Photon
        TColor::GetColor("#ff7f00"), // ChargedPion
        TColor::GetColor("#984ea3"), // NeutralPion
        TColor::GetColor("#ffff33"), // Neutron
        TColor::GetColor("#1b9e77"), // Proton
        TColor::GetColor("#f781bf"), // ChargedKaon
        TColor::GetColor("#a65628"), // NeutralKaon
        TColor::GetColor("#66a61e"), // Lambda
        TColor::GetColor("#e6ab02"), // ChargedSigma
        TColor::GetColor("#a6cee3"), // NeutralSigma
        TColor::GetColor("#b15928")  // Other
    };

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
    // Wide legend across the top of the canvas
    legend_ = std::make_unique<TLegend>(0.02, 0.89, 0.98, 0.99);
    legend_->SetNColumns(5);
    legend_->SetFillColor(background);
    legend_->SetFillStyle(1001);
    legend_->SetBorderSize(0);
    legend_->SetTextFont(42);
    legend_->SetTextSize(0.025);

    const std::array<const char *, palette_size> labels = {
        "#emptyset",    "Cosmic",     "#mu",  "e^{-}",   "#gamma", "#pi^{#pm}",
        "#pi^{0}",      "n",          "p",    "K^{#pm}", "K^{0}",  "#Lambda",
        "#Sigma^{#pm}", "#Sigma^{0}", "Other"};

    const std::array<Style_t, palette_size> styles = {
        1001, 3004, 1001, 1001, 1001, 3005, 1001, 3354,
        1001, 3002, 1001, 1001, 3003, 1001, 1001};
    for (int i = 0; i < palette_size; ++i) {
      auto h = std::make_unique<TH1F>((tag_ + std::to_string(i)).c_str(), "", 1,
                                      0, 1);
      h->SetFillColor(palette[i]);
      h->SetLineColor(palette[i]);
      h->SetLineWidth(1);
      h->SetFillStyle(styles[i]);
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
