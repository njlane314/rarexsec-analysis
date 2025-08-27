#ifndef PLOT_CATALOG_H
#define PLOT_CATALOG_H

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisTypes.h"
#include "HistogramCut.h"
#include "OccupancyMatrixPlot.h"
#include "RegionAnalysis.h"
#include "Selection.h"
#include "StackedHistogramPlot.h"
#include "UnstackedHistogramPlot.h"

namespace analysis {

class PlotCatalog {
  public:
    PlotCatalog(AnalysisDataLoader &loader, int image_size, const std::string &output_directory = "./plots")
        : loader_(loader), image_size_(image_size) {
        auto p = std::filesystem::absolute(output_directory).lexically_normal();
        std::filesystem::create_directories(p);
        output_directory_ = p;
    }

    void generateStackedPlot(const RegionAnalysisMap &phase_space, const std::string &variable,
                             const std::string &region, const std::string &category_column, bool overlay_signal = true,
                             const std::vector<Cut> &cut_list = {}, bool annotate_numbers = true) const {
        const auto &result = this->fetchResult(phase_space, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "stacked_" + sanitise(variable) + "_" + sanitise(region.empty() ? "default" : region) + "_" +
                           sanitise(category_column);

        const RegionAnalysis &region_info = phase_space.at(RegionKey{region});

        StackedHistogramPlot plot(std::move(name), result, region_info, category_column, output_directory_.string(),
                                  overlay_signal, cut_list, annotate_numbers);
        plot.drawAndSave();
    }

    void generateUnstackedPlot(const RegionAnalysisMap &phase_space, const std::string &variable,
                               const std::string &region, const std::string &category_column,
                               const std::vector<Cut> &cut_list = {}, bool annotate_numbers = true,
                               bool area_normalise = false, bool use_log_y = false,
                               const std::string &y_axis_label = "Events") const {
        const auto &result = this->fetchResult(phase_space, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "unstacked_" + sanitise(variable) + "_" + sanitise(region.empty() ? "default" : region) +
                           "_" + sanitise(category_column);

        const RegionAnalysis &region_info = phase_space.at(RegionKey{region});

        UnstackedHistogramPlot plot(std::move(name), result, region_info, category_column, output_directory_.string(),
                                    cut_list, annotate_numbers, use_log_y, y_axis_label, area_normalise);
        plot.drawAndSave();
    }

    void generateOccupancyMatrixPlot(const RegionAnalysisMap &phase_space, const std::string &x_variable,
                                     const std::string &y_variable, const std::string &region,
                                     const Selection &selection, const std::vector<Cut> &x_cuts = {},
                                     const std::vector<Cut> &y_cuts = {}) const {
        const auto &x_res = this->fetchResult(phase_space, x_variable, region);
        const auto &y_res = this->fetchResult(phase_space, y_variable, region);

        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "occupancy_matrix_" + sanitise(x_variable) + "_vs_" + sanitise(y_variable) + "_" +
                           sanitise(region.empty() ? "default" : region);

        auto x_edges = x_res.binning_.getEdges();
        auto y_edges = y_res.binning_.getEdges();

        TH2F *hist = new TH2F(name.c_str(), name.c_str(), x_edges.size() - 1, x_edges.data(), y_edges.size() - 1,
                              y_edges.data());
        hist->GetXaxis()->SetTitle(x_res.binning_.getTexLabel().c_str());
        hist->GetYaxis()->SetTitle(y_res.binning_.getTexLabel().c_str());

        std::string filter = selection.str();

        bool has_filter = filter.find_first_not_of(" \t\n\r") != std::string::npos;

        for (auto &kv : loader_.getSampleFrames()) {
            auto &sample = kv.second;
            auto df = sample.nominal_node_;
            if (has_filter) {
                df = df.Filter(filter);
            }
            for (auto const &c : x_cuts) {
                std::string expr = x_res.binning_.getVariable() +
                                   (c.direction == CutDirection::GreaterThan ? ">" : "<") + std::to_string(c.threshold);
                df = df.Filter(expr);
            }
            for (auto const &c : y_cuts) {
                std::string expr = y_res.binning_.getVariable() +
                                   (c.direction == CutDirection::GreaterThan ? ">" : "<") + std::to_string(c.threshold);
                df = df.Filter(expr);
            }

            bool has_w = df.HasColumn("nominal_event_weight");
            const auto &x_col = x_res.binning_.getVariable();
            const auto &y_col = y_res.binning_.getVariable();

            auto x_type = df.GetColumnType(x_col);
            auto y_type = df.GetColumnType(y_col);
            bool x_is_vec = x_type.find("vector") != std::string::npos || x_type.find("RVec") != std::string::npos;
            bool y_is_vec = y_type.find("vector") != std::string::npos || y_type.find("RVec") != std::string::npos;

            std::vector<double> ws;
            size_t n_events = df.Count().GetValue();
            if (has_w) {
                auto w_type = df.GetColumnType("nominal_event_weight");
                if (w_type.find("float") != std::string::npos) {
                    auto wv = df.Take<float>("nominal_event_weight").GetValue();
                    ws.assign(wv.begin(), wv.end());
                } else {
                    ws = df.Take<double>("nominal_event_weight").GetValue();
                }
            } else {
                ws.assign(n_events, 1.0);
            }

            auto get_scalar = [&](const std::string &col, const std::string &type) {
                std::vector<double> vals;
                if (type.find("float") != std::string::npos) {
                    auto v = df.Take<float>(col).GetValue();
                    vals.assign(v.begin(), v.end());
                } else if (type.find("int") != std::string::npos || type.find("unsigned") != std::string::npos) {
                    auto v = df.Take<int>(col).GetValue();
                    vals.assign(v.begin(), v.end());
                } else {
                    vals = df.Take<double>(col).GetValue();
                }
                return vals;
            };

            auto get_vector = [&](const std::string &col, const std::string &type) {
                std::vector<std::vector<double>> vals;
                if (type.find("float") != std::string::npos) {
                    auto v = df.Take<std::vector<float>>(col).GetValue();
                    for (auto &inner : v)
                        vals.emplace_back(inner.begin(), inner.end());
                } else if (type.find("int") != std::string::npos || type.find("unsigned") != std::string::npos) {
                    auto v = df.Take<std::vector<int>>(col).GetValue();
                    for (auto &inner : v)
                        vals.emplace_back(inner.begin(), inner.end());
                } else {
                    vals = df.Take<std::vector<double>>(col).GetValue();
                }
                return vals;
            };

            if (!x_is_vec && !y_is_vec) {
                auto xs = get_scalar(x_col, x_type);
                auto ys = get_scalar(y_col, y_type);
                for (size_t i = 0; i < xs.size(); ++i) {
                    hist->Fill(xs[i], ys[i], ws[i]);
                }
            } else if (x_is_vec && !y_is_vec) {
                auto xs = get_vector(x_col, x_type);
                auto ys = get_scalar(y_col, y_type);
                for (size_t i = 0; i < xs.size(); ++i) {
                    for (double xv : xs[i]) {
                        hist->Fill(xv, ys[i], ws[i]);
                    }
                }
            } else if (!x_is_vec && y_is_vec) {
                auto xs = get_scalar(x_col, x_type);
                auto ys = get_vector(y_col, y_type);
                for (size_t i = 0; i < ys.size(); ++i) {
                    for (double yv : ys[i]) {
                        hist->Fill(xs[i], yv, ws[i]);
                    }
                }
            } else {
                auto xs = get_vector(x_col, x_type);
                auto ys = get_vector(y_col, y_type);
                for (size_t i = 0; i < xs.size(); ++i) {
                    size_t lim = std::min(xs[i].size(), ys[i].size());
                    for (size_t j = 0; j < lim; ++j) {
                        hist->Fill(xs[i][j], ys[i][j], ws[i]);
                    }
                }
            }
        }

        OccupancyMatrixPlot plot(std::move(name), hist, output_directory_.string());
        plot.drawAndSave();
    }

  private:
    const VariableResult &fetchResult(const RegionAnalysisMap &phase_space, const std::string &variable,
                                      const std::string &region) const {
        RegionKey rkey{region};
        VariableKey vkey{variable};
        auto rit = phase_space.find(rkey);
        if (rit != phase_space.end()) {
            if (rit->second.hasFinalVariable(vkey)) {
                return rit->second.getFinalVariable(vkey);
            }
            log::fatal("PlotCatalog::fetchResult", "Missing analysis result for variable", variable, "in region",
                       region);
            throw std::runtime_error("Missing analysis result for variable");
        }
        log::fatal("PlotCatalog::fetchResult", "Missing analysis result for region", region);
        throw std::runtime_error("Missing analysis result for region");
    }

    AnalysisDataLoader &loader_;
    int image_size_;
    std::filesystem::path output_directory_;
};

}

#endif
