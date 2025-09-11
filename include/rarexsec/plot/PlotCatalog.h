#ifndef PLOT_CATALOG_H
#define PLOT_CATALOG_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <rarexsec/data/AnalysisDataLoader.h>
#include <rarexsec/core/AnalysisResult.h>
#include <rarexsec/plot/HistogramCut.h>
#include <rarexsec/plot/IHistogramPlot.h>
#include <rarexsec/plot/MatrixPlot.h>
#include <rarexsec/core/SelectionQuery.h>
#include <rarexsec/plot/StackedHistogramPlot.h>
#include <rarexsec/plot/UnstackedHistogramPlot.h>

namespace analysis {

class PlotCatalog {
  public:
    PlotCatalog(AnalysisDataLoader &loader, int image_size,
                const std::string &output_directory = "./plots")
        : loader_(loader), image_size_(image_size) {
        auto p = std::filesystem::absolute(output_directory).lexically_normal();
        std::filesystem::create_directories(p);
        output_directory_ = p;
    }

    void generateStackedPlot(const AnalysisResult &res,
                             const std::string &variable,
                             const std::string &region,
                             const std::string &category_column,
                             bool overlay_signal = true,
                             const std::string &signal_group =
                                 "inclusive_strange_channels",
                             const std::vector<Cut> &cut_list = {},
                             bool annotate_numbers = true) const {
        const auto &result = this->fetchResult(res, variable, region);
        std::string name =
            "stacked_" + IHistogramPlot::sanitise(variable) + "_" +
            IHistogramPlot::sanitise(region.empty() ? "default" : region) + "_" +
            IHistogramPlot::sanitise(category_column);

        const RegionAnalysis &region_info = res.region(RegionKey{region});

        StackedHistogramPlot plot(std::move(name), result, region_info,
                                  category_column, output_directory_.string(),
                                  overlay_signal, signal_group, cut_list,
                                  annotate_numbers);
        plot.drawAndSave();
    }

    void generateUnstackedPlot(
        const AnalysisResult &res, const std::string &variable,
        const std::string &region, const std::string &category_column,
        const std::vector<Cut> &cut_list = {}, bool annotate_numbers = true,
        bool area_normalise = false, bool use_log_y = false,
        const std::string &y_axis_label = "Events") const {
        const auto &result = this->fetchResult(res, variable, region);
        std::string name =
            "unstacked_" + IHistogramPlot::sanitise(variable) + "_" +
            IHistogramPlot::sanitise(region.empty() ? "default" : region) + "_" +
            IHistogramPlot::sanitise(category_column);

        const RegionAnalysis &region_info = res.region(RegionKey{region});

        UnstackedHistogramPlot plot(std::move(name), result, region_info,
                                    category_column, output_directory_.string(),
                                    cut_list, annotate_numbers, use_log_y,
                                    y_axis_label, area_normalise);
        plot.drawAndSave();
    }

    void generateMatrixPlot(const AnalysisResult &res,
                            const std::string &x_variable,
                            const std::string &y_variable,
                            const std::string &region,
                            const SelectionQuery &selection,
                            const std::vector<Cut> &x_cuts = {},
                            const std::vector<Cut> &y_cuts = {}) const {
        const auto &x_res = this->fetchResult(res, x_variable, region);
        const auto &y_res = this->fetchResult(res, y_variable, region);

        std::string name =
            "occupancy_matrix_" + IHistogramPlot::sanitise(x_variable) + "_vs_" +
            IHistogramPlot::sanitise(y_variable) + "_" +
            IHistogramPlot::sanitise(region.empty() ? "default" : region);

        MatrixPlot plot(std::move(name), x_res, y_res, loader_, selection,
                        output_directory_.string(), x_cuts, y_cuts);
        plot.drawAndSave();
    }

  private:
    const VariableResult &fetchResult(const AnalysisResult &res,
                                      const std::string &variable,
                                      const std::string &region) const {
        RegionKey rkey{region};
        VariableKey vkey{variable};
        if (res.hasResult(rkey, vkey)) {
            return res.result(rkey, vkey);
        }
        log::fatal("PlotCatalog::fetchResult",
                    "Missing analysis result for variable", variable, "in region",
                    region);
        throw std::runtime_error("Missing analysis result for variable");
    }

    AnalysisDataLoader &loader_;
    int image_size_;
    std::filesystem::path output_directory_;
};

}  

#endif
