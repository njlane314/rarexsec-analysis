#ifndef PLOT_CATALOG_H
#define PLOT_CATALOG_H

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AnalysisDataLoader.h"
#include "AnalysisResult.h"
#include "HistogramCut.h"
#include "OccupancyMatrixPlot.h"
#include "QuadTreeBinning2D.h"
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

    void generateStackedPlot(const AnalysisResult &res, const std::string &variable, const std::string &region,
                             const std::string &category_column, bool overlay_signal = true,
                             const std::vector<Cut> &cut_list = {}, bool annotate_numbers = true) const {
        const auto &result = this->fetchResult(res, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "stacked_" + sanitise(variable) + "_" + sanitise(region.empty() ? "default" : region) + "_" +
                           sanitise(category_column);

        const RegionAnalysis &region_info = res.region(RegionKey{region});

        StackedHistogramPlot plot(std::move(name), result, region_info, category_column, output_directory_.string(),
                                  overlay_signal, cut_list, annotate_numbers);
        plot.drawAndSave();
    }

    void generateUnstackedPlot(const AnalysisResult &res, const std::string &variable, const std::string &region,
                               const std::string &category_column, const std::vector<Cut> &cut_list = {},
                               bool annotate_numbers = true, bool area_normalise = false, bool use_log_y = false,
                               const std::string &y_axis_label = "Events") const {
        const auto &result = this->fetchResult(res, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "unstacked_" + sanitise(variable) + "_" + sanitise(region.empty() ? "default" : region) +
                           "_" + sanitise(category_column);

        const RegionAnalysis &region_info = res.region(RegionKey{region});

        UnstackedHistogramPlot plot(std::move(name), result, region_info, category_column, output_directory_.string(),
                                    cut_list, annotate_numbers, use_log_y, y_axis_label, area_normalise);
        plot.drawAndSave();
    }

    void generateOccupancyMatrixPlot(const AnalysisResult &res, const std::string &x_variable,
                                     const std::string &y_variable, const std::string &region,
                                     const Selection &selection, const std::vector<Cut> &x_cuts = {},
                                     const std::vector<Cut> &y_cuts = {}) const {
        const auto &x_res = this->fetchResult(res, x_variable, region);
        const auto &y_res = this->fetchResult(res, y_variable, region);

        auto sanitise = [&](std::string s) {
            for (auto &c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name = "occupancy_matrix_" + sanitise(x_variable) + "_vs_" + sanitise(y_variable) + "_" +
                           sanitise(region.empty() ? "default" : region);

        auto edges = determineEdges(x_res, y_res);

        TH2F *hist = new TH2F(name.c_str(), name.c_str(), edges.first.size() - 1, edges.first.data(),
                              edges.second.size() - 1, edges.second.data());
        hist->GetXaxis()->SetTitle(x_res.binning_.getTexLabel().c_str());
        hist->GetYaxis()->SetTitle(y_res.binning_.getTexLabel().c_str());

        std::string filter = selection.str();

        for (auto &kv : loader_.getSampleFrames()) {
            auto df = kv.second.nominal_node_;
            df = applySelectionFilters(df, x_res, y_res, filter, x_cuts, y_cuts);
            auto data = extractValues(df, x_res, y_res);
            fillHistogram(hist, data);
        }

        OccupancyMatrixPlot plot(std::move(name), hist, output_directory_.string());
        plot.drawAndSave();
    }

  private:
    const VariableResult &fetchResult(const AnalysisResult &res, const std::string &variable,
                                      const std::string &region) const {
        RegionKey rkey{region};
        VariableKey vkey{variable};
        if (res.hasResult(rkey, vkey)) {
            return res.result(rkey, vkey);
        }
        log::fatal("PlotCatalog::fetchResult", "Missing analysis result for variable", variable, "in region", region);
        throw std::runtime_error("Missing analysis result for variable");
    }

    std::pair<std::vector<double>, std::vector<double>> determineEdges(const VariableResult &x_res,
                                                                        const VariableResult &y_res) const {
        auto x_edges = x_res.binning_.getEdges();
        auto y_edges = y_res.binning_.getEdges();

        std::vector<ROOT::RDF::RNode> mc_nodes;
        for (auto &[key, sample] : loader_.getSampleFrames())
            if (sample.isMc())
                mc_nodes.emplace_back(sample.nominal_node_);
        if (!mc_nodes.empty()) {
            auto bins = QuadTreeBinning2D::calculate(mc_nodes, x_res.binning_, y_res.binning_);
            x_edges = bins.first.getEdges();
            y_edges = bins.second.getEdges();
        }

        return {x_edges, y_edges};
    }

    ROOT::RDF::RNode applySelectionFilters(ROOT::RDF::RNode df, const VariableResult &x_res,
                                           const VariableResult &y_res, const std::string &filter,
                                           const std::vector<Cut> &x_cuts, const std::vector<Cut> &y_cuts) const {
        if (filter.find_first_not_of(" \t\n\r") != std::string::npos)
            df = df.Filter(filter);

        for (auto const &c : x_cuts) {
            std::string expr = x_res.binning_.getVariable() +
                               (c.direction == CutDirection::GreaterThan ? ">" : "<") +
                               std::to_string(c.threshold);
            df = df.Filter(expr);
        }

        for (auto const &c : y_cuts) {
            std::string expr = y_res.binning_.getVariable() +
                               (c.direction == CutDirection::GreaterThan ? ">" : "<") +
                               std::to_string(c.threshold);
            df = df.Filter(expr);
        }

        return df;
    }

    struct SampleData {
        bool x_is_vec;
        bool y_is_vec;
        std::vector<double> ws;
        std::vector<double> xs;
        std::vector<double> ys;
        std::vector<std::vector<double>> xs_vec;
        std::vector<std::vector<double>> ys_vec;
    };

    SampleData extractValues(ROOT::RDF::RNode df, const VariableResult &x_res,
                             const VariableResult &y_res) const {
        SampleData data;

        bool has_w = df.HasColumn("nominal_event_weight");
        const auto &x_col = x_res.binning_.getVariable();
        const auto &y_col = y_res.binning_.getVariable();

        auto x_type = df.GetColumnType(x_col);
        auto y_type = df.GetColumnType(y_col);
        data.x_is_vec = x_type.find("vector") != std::string::npos || x_type.find("RVec") != std::string::npos;
        data.y_is_vec = y_type.find("vector") != std::string::npos || y_type.find("RVec") != std::string::npos;

        size_t n_events = df.Count().GetValue();
        if (has_w) {
            auto w_type = df.GetColumnType("nominal_event_weight");
            if (w_type.find("float") != std::string::npos) {
                auto wv = df.Take<float>("nominal_event_weight").GetValue();
                data.ws.assign(wv.begin(), wv.end());
            } else {
                data.ws = df.Take<double>("nominal_event_weight").GetValue();
            }
        } else {
            data.ws.assign(n_events, 1.0);
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

        if (!data.x_is_vec)
            data.xs = get_scalar(x_col, x_type);
        else
            data.xs_vec = get_vector(x_col, x_type);

        if (!data.y_is_vec)
            data.ys = get_scalar(y_col, y_type);
        else
            data.ys_vec = get_vector(y_col, y_type);

        return data;
    }

    void fillHistogram(TH2F *hist, const SampleData &data) const {
        if (!data.x_is_vec && !data.y_is_vec) {
            for (size_t i = 0; i < data.xs.size(); ++i)
                hist->Fill(data.xs[i], data.ys[i], data.ws[i]);
        } else if (data.x_is_vec && !data.y_is_vec) {
            for (size_t i = 0; i < data.xs_vec.size(); ++i)
                for (double xv : data.xs_vec[i])
                    hist->Fill(xv, data.ys[i], data.ws[i]);
        } else if (!data.x_is_vec && data.y_is_vec) {
            for (size_t i = 0; i < data.ys_vec.size(); ++i)
                for (double yv : data.ys_vec[i])
                    hist->Fill(data.xs[i], yv, data.ws[i]);
        } else {
            for (size_t i = 0; i < data.xs_vec.size(); ++i) {
                size_t lim = std::min(data.xs_vec[i].size(), data.ys_vec[i].size());
                for (size_t j = 0; j < lim; ++j)
                    hist->Fill(data.xs_vec[i][j], data.ys_vec[i][j], data.ws[i]);
            }
        }
    }

    AnalysisDataLoader &loader_;
    int image_size_;
    std::filesystem::path output_directory_;
};

}

#endif
