#ifndef PLOT_CATALOG_H
#define PLOT_CATALOG_H

#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisResult.h"
#include "EventDisplay.h"
#include "StackedHistogramPlot.h"
#include "TSystem.h"

namespace analysis {

class PlotCatalog {
public:
  PlotCatalog(AnalysisDataLoader &loader, int image_size,
              const std::string &output_directory = "plots")
      : loader_(loader), image_size_(image_size),
        output_directory_(output_directory) {
    gSystem->mkdir(output_directory_.c_str(), true);
  }

  void generateStackedPlot(const AnalysisPhaseSpace &phase_space,
                           const std::string &variable,
                           const std::string &region,
                           const std::string &category_column,
                           bool overlay_signal = true,
                           const std::vector<Cut> &cut_list = {},
                           bool annotate_numbers = true) const {
    const auto &result = this->fetchResult(phase_space, variable, region);
    auto sanitise = [&](std::string s) {
      for (auto &c : s)
        if (c == '.' || c == '/' || c == ' ')
          c = '_';
      return s;
    };
    std::string name = "stacked_" + sanitise(variable) + "_" +
                       sanitise(region.empty() ? "default" : region) + "_" +
                       sanitise(category_column);

    StackedHistogramPlot plot(std::move(name), result, category_column,
                              output_directory_, overlay_signal, cut_list,
                              annotate_numbers);
    plot.renderAndSave();
  }

  void generateEventDisplay(const SampleEvent &sample_event,
                            const std::string &sample_key) const {
    EventDisplay vis(loader_, image_size_, output_directory_);
    vis.visualiseEvent(sample_event, sample_key);
  }

private:
  const AnalysisResult &fetchResult(const AnalysisPhaseSpace &phase_space,
                                    const std::string &variable,
                                    const std::string &region) const {
    std::string key = variable + "@" + region;
    auto it = phase_space.find(key);
    if (it != phase_space.end())
      return it->second;
    log::fatal("PlotCatalog", "Missing analysis result for key", key);
  }

  AnalysisDataLoader &loader_;
  int image_size_;
  std::string output_directory_;
};

}

#endif