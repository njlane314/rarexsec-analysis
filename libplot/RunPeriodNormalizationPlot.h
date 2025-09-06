#ifndef RUNPERIODNORMALIZATIONPLOT_H
#define RUNPERIODNORMALIZATIONPLOT_H

#include <map>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TSystem.h"

namespace analysis {

class RunPeriodNormalizationPlot {
 public:
  RunPeriodNormalizationPlot(std::string plot_name, AnalysisDataLoader &loader,
                             std::string run_column, std::string pot_column,
                             std::string trig_column, std::string ext_trig_column,
                             std::string output_directory = "plots")
      : plot_name_(std::move(plot_name)), loader_(loader), run_column_(std::move(run_column)),
        pot_column_(std::move(pot_column)), trig_column_(std::move(trig_column)),
        ext_trig_column_(std::move(ext_trig_column)), output_directory_(std::move(output_directory)) {
    gSystem->mkdir(output_directory_.c_str(), true);
  }

  void drawAndSave() const {
    std::map<int, double> pot_map;
    std::map<int, long> trig_map;
    std::map<int, long> ext_map;
    std::map<int, long> count_map;

    for (auto const &[skey, sample] : loader_.getSampleFrames()) {
      auto df = sample.nominal_node_;
      auto runs = df.Take<int>(run_column_).GetValue();
      auto pots = df.Take<double>(pot_column_).GetValue();
      auto trigs = df.Take<long>(trig_column_).GetValue();
      auto exts = df.Take<long>(ext_trig_column_).GetValue();
      size_t n = runs.size();
      for (size_t i = 0; i < n; ++i) {
        int r = runs[i];
        pot_map[r] += pots[i];
        trig_map[r] += trigs[i];
        ext_map[r] += exts[i];
        count_map[r] += 1;
      }
    }

    size_t n = pot_map.size();
    std::vector<double> run_vals; run_vals.reserve(n);
    std::vector<double> pot_vals; pot_vals.reserve(n);
    std::vector<double> trig_vals; trig_vals.reserve(n);
    std::vector<double> ext_vals; ext_vals.reserve(n);
    std::vector<double> cnt_vals; cnt_vals.reserve(n);

    for (auto const &kv : pot_map) {
      run_vals.push_back(static_cast<double>(kv.first));
      pot_vals.push_back(kv.second);
      trig_vals.push_back(static_cast<double>(trig_map[kv.first]));
      ext_vals.push_back(static_cast<double>(ext_map[kv.first]));
      cnt_vals.push_back(static_cast<double>(count_map[kv.first]));
    }

    TCanvas c1;
    TGraph g1(n, run_vals.data(), pot_vals.data());
    g1.SetTitle("POT vs Run;Run;POT");
    g1.Draw("APL");
    c1.SaveAs((output_directory_ + "/" + plot_name_ + "_pot.pdf").c_str());

    TCanvas c2;
    TGraph g2(n, run_vals.data(), trig_vals.data());
    g2.SetTitle("Triggers vs Run;Run;Triggers");
    g2.Draw("APL");
    c2.SaveAs((output_directory_ + "/" + plot_name_ + "_trig.pdf").c_str());

    TCanvas c3;
    TGraph g3(n, run_vals.data(), ext_vals.data());
    g3.SetTitle("Ext Trig vs Run;Run;Ext Trig");
    g3.Draw("APL");
    c3.SaveAs((output_directory_ + "/" + plot_name_ + "_ext.pdf").c_str());

    TCanvas c4;
    TGraph g4(n, run_vals.data(), cnt_vals.data());
    g4.SetTitle("Events vs Run;Run;Events");
    g4.Draw("APL");
    c4.SaveAs((output_directory_ + "/" + plot_name_ + "_events.pdf").c_str());
  }

 private:
  std::string plot_name_;
  AnalysisDataLoader &loader_;
  std::string run_column_;
  std::string pot_column_;
  std::string trig_column_;
  std::string ext_trig_column_;
  std::string output_directory_;
};

} // namespace analysis

#endif
