#ifndef OCCUPANCYMATRIXPLOT_H
#define OCCUPANCYMATRIXPLOT_H

#include <string>
#include <utility>
#include <vector>

#include "ROOT/RDataFrame.hxx"

#include "AnalysisDataLoader.h"
#include "AnalysisTypes.h"
#include "HistogramCut.h"
#include "QuadTreeBinning2D.h"
#include "Selection.h"

#include "TCanvas.h"
#include "TH2F.h"
#include "TStyle.h"

#include "IHistogramPlot.h"

namespace analysis {

class OccupancyMatrixPlot : public IHistogramPlot {
  public:
    OccupancyMatrixPlot(std::string plot_name, AnalysisDataLoader &loader, const VariableResult &x_res,
                        const VariableResult &y_res, const Selection &selection, std::vector<Cut> x_cuts = {},
                        std::vector<Cut> y_cuts = {}, std::string output_directory = "plots")
        : IHistogramPlot(std::move(plot_name), std::move(output_directory)), loader_(loader), x_res_(x_res),
          y_res_(y_res), selection_(selection), x_cuts_(std::move(x_cuts)), y_cuts_(std::move(y_cuts)) {
        auto edges = determineEdges();
        hist_ = new TH2F(plot_name_.c_str(), plot_name_.c_str(), edges.first.size() - 1, edges.first.data(),
                         edges.second.size() - 1, edges.second.data());
        hist_->GetXaxis()->SetTitle(x_res_.binning_.getTexLabel().c_str());
        hist_->GetYaxis()->SetTitle(y_res_.binning_.getTexLabel().c_str());

        fillFromSamples();
    }

    ~OccupancyMatrixPlot() override { delete hist_; }

  private:
    std::pair<std::vector<double>, std::vector<double>> determineEdges() const {
        auto x_edges = x_res_.binning_.getEdges();
        auto y_edges = y_res_.binning_.getEdges();

        std::vector<ROOT::RDF::RNode> mc_nodes;
        for (auto &[key, sample] : loader_.getSampleFrames())
            if (sample.isMc())
                mc_nodes.emplace_back(sample.nominal_node_);
        if (!mc_nodes.empty()) {
            auto bins = QuadTreeBinning2D::calculate(mc_nodes, x_res_.binning_, y_res_.binning_);
            x_edges = bins.first.getEdges();
            y_edges = bins.second.getEdges();
        }

        return {x_edges, y_edges};
    }

    ROOT::RDF::RNode applySelectionFilters(ROOT::RDF::RNode df) const {
        std::string filter = selection_.str();
        if (filter.find_first_not_of(" \t\n\r") != std::string::npos)
            df = df.Filter(filter);

        for (auto const &c : x_cuts_) {
            std::string expr = x_res_.binning_.getVariable() +
                               (c.direction == CutDirection::GreaterThan ? ">" : "<") +
                               std::to_string(c.threshold);
            df = df.Filter(expr);
        }

        for (auto const &c : y_cuts_) {
            std::string expr = y_res_.binning_.getVariable() +
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

    SampleData extractValues(ROOT::RDF::RNode df) const {
        SampleData data;

        bool has_w = df.HasColumn("nominal_event_weight");
        const auto &x_col = x_res_.binning_.getVariable();
        const auto &y_col = y_res_.binning_.getVariable();

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

    void fillHistogram(const SampleData &data) const {
        if (!data.x_is_vec && !data.y_is_vec) {
            for (size_t i = 0; i < data.xs.size(); ++i)
                hist_->Fill(data.xs[i], data.ys[i], data.ws[i]);
        } else if (data.x_is_vec && !data.y_is_vec) {
            for (size_t i = 0; i < data.xs_vec.size(); ++i)
                for (double xv : data.xs_vec[i])
                    hist_->Fill(xv, data.ys[i], data.ws[i]);
        } else if (!data.x_is_vec && data.y_is_vec) {
            for (size_t i = 0; i < data.ys_vec.size(); ++i)
                for (double yv : data.ys_vec[i])
                    hist_->Fill(data.xs[i], yv, data.ws[i]);
        } else {
            for (size_t i = 0; i < data.xs_vec.size(); ++i) {
                size_t lim = std::min(data.xs_vec[i].size(), data.ys_vec[i].size());
                for (size_t j = 0; j < lim; ++j)
                    hist_->Fill(data.xs_vec[i][j], data.ys_vec[i][j], data.ws[i]);
            }
        }
    }

    void fillFromSamples() {
        for (auto &kv : loader_.getSampleFrames()) {
            auto df = kv.second.nominal_node_;
            df = applySelectionFilters(df);
            auto data = extractValues(df);
            fillHistogram(data);
        }
    }

    AnalysisDataLoader &loader_;
    const VariableResult &x_res_;
    const VariableResult &y_res_;
    Selection selection_;
    std::vector<Cut> x_cuts_;
    std::vector<Cut> y_cuts_;
    TH2F *hist_ = nullptr;
};

}

#endif

