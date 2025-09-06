#ifndef SLIPSTACKINGINTENSITYPLOT_H
#define SLIPSTACKINGINTENSITYPLOT_H

#include <map>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TLegend.h"
#include "TSystem.h"

namespace analysis {

class SlipStackingIntensityPlot {
 public:
  SlipStackingIntensityPlot(std::string plot_name, AnalysisDataLoader &loader,
                             std::string run_column, std::string pot4p6_column,
                             std::string pot6p6_column, std::string other_column,
                             std::string output_directory = "plots")
      : plot_name_(std::move(plot_name)), loader_(loader), run_column_(std::move(run_column)),
        pot4p6_column_(std::move(pot4p6_column)), pot6p6_column_(std::move(pot6p6_column)),
        other_column_(std::move(other_column)), output_directory_(std::move(output_directory)) {
    gSystem->mkdir(output_directory_.c_str(), true);
  }

  void drawAndSave() const {
    std::map<int, double> pot4p6_map;
    std::map<int, double> pot6p6_map;
    std::map<int, double> other_map;

    for (auto const &[skey, sample] : loader_.getSampleFrames()) {
      auto df = sample.nominal_node_;
      auto runs = df.Take<int>(run_column_).GetValue();
      auto p4 = df.Take<double>(pot4p6_column_).GetValue();
      auto p6 = df.Take<double>(pot6p6_column_).GetValue();
      auto po = df.Take<double>(other_column_).GetValue();
      size_t n = runs.size();
      for (size_t i = 0; i < n; ++i) {
        int r = runs[i];
        pot4p6_map[r] += p4[i];
        pot6p6_map[r] += p6[i];
        other_map[r] += po[i];
      }
    }

    size_t n = pot4p6_map.size();
    std::vector<double> run_vals; run_vals.reserve(n);
    std::vector<double> pot4p6_vals; pot4p6_vals.reserve(n);
    std::vector<double> pot6p6_vals; pot6p6_vals.reserve(n);
    std::vector<double> other_vals; other_vals.reserve(n);

    for (auto const &kv : pot4p6_map) {
      run_vals.push_back(static_cast<double>(kv.first));
      pot4p6_vals.push_back(kv.second);
      pot6p6_vals.push_back(pot6p6_map[kv.first]);
      other_vals.push_back(other_map[kv.first]);
    }

    TCanvas c1;
    TGraph g1(n, run_vals.data(), pot4p6_vals.data());
    g1.SetLineColor(2);
    g1.SetTitle("POT vs Run;Run;POT");
    g1.Draw("AL");
    TGraph g2(n, run_vals.data(), pot6p6_vals.data());
    g2.SetLineColor(4);
    g2.Draw("L same");
    TGraph g3(n, run_vals.data(), other_vals.data());
    g3.SetLineColor(8);
    g3.Draw("L same");
    TLegend l(0.7, 0.7, 0.9, 0.9);
    l.AddEntry(&g1, "pot4p6", "l");
    l.AddEntry(&g2, "pot6p6", "l");
    l.AddEntry(&g3, "other", "l");
    l.Draw();
    c1.SaveAs((output_directory_ + "/" + plot_name_ + ".pdf").c_str());
  }

 private:
  std::string plot_name_;
  AnalysisDataLoader &loader_;
  std::string run_column_;
  std::string pot4p6_column_;
  std::string pot6p6_column_;
  std::string other_column_;
  std::string output_directory_;
};

} // namespace analysis

#endif
