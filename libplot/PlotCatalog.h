#ifndef PLOT_CATALOG_H
#define PLOT_CATALOG_H

#include <stdexcept>
#include <string>
#include <vector>
#include <random>
#include <numeric>

#include "TSystem.h"

#include "AnalysisDataLoader.h"
#include "RegionAnalysis.h"
#include "AnalysisTypes.h"
#include "EventDisplay.h"
#include "StackedHistogramPlot.h"
#include "UnstackedHistogramPlot.h"
#include "HeatmapPlot.h"
#include "HistogramCut.h"
#include "Selection.h"

namespace analysis {

class PlotCatalog {
public:
    PlotCatalog(AnalysisDataLoader& loader,
                int image_size,
                const std::string& output_directory = "plots")
      : loader_(loader)
      , image_size_(image_size)
      , output_directory_(output_directory)
    {
        gSystem->mkdir(output_directory_.c_str(), true);
    }

    void generateStackedPlot(
        const RegionAnalysisMap& phase_space,
        const std::string&       variable,
        const std::string&       region,
        const std::string&       category_column,
        bool                     overlay_signal = true,
        const std::vector<Cut>&  cut_list = {},
        bool                     annotate_numbers = true) const
    {
        const auto& result = this->fetchResult(phase_space, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto& c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name =
            "stacked_" + sanitise(variable) + "_" +
            sanitise(region.empty() ? "default" : region) + "_" +
            sanitise(category_column);

        const RegionAnalysis& region_info = phase_space.at(RegionKey{region});

        StackedHistogramPlot plot(
            std::move(name),
            result,
            region_info,
            category_column,
            output_directory_,
            overlay_signal,
            cut_list,
            annotate_numbers
        );
        plot.drawAndSave();
    }

    void generateUnstackedPlot(
        const RegionAnalysisMap& phase_space,
        const std::string&       variable,
        const std::string&       region,
        const std::string&       category_column,
        const std::vector<Cut>&  cut_list = {},
        bool                     annotate_numbers = true,
        bool                     area_normalise = false,
        bool                     use_log_y = false,
        const std::string&       y_axis_label = "Events") const
    {
        const auto& result = this->fetchResult(phase_space, variable, region);
        auto sanitise = [&](std::string s) {
            for (auto& c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name =
            "unstacked_" + sanitise(variable) + "_" +
            sanitise(region.empty() ? "default" : region) + "_" +
            sanitise(category_column);

        const RegionAnalysis& region_info = phase_space.at(RegionKey{region});

        UnstackedHistogramPlot plot(
            std::move(name),
            result,
            region_info,
            category_column,
            output_directory_,
            cut_list,
            annotate_numbers,
            use_log_y,
            y_axis_label,
            area_normalise
        );
        plot.drawAndSave();
    }

    void generateHeatmapPlot(
        const RegionAnalysisMap& phase_space,
        const std::string&       x_variable,
        const std::string&       y_variable,
        const std::string&       region,
        const Selection&         selection,
        const std::vector<Cut>&  x_cuts = {},
        const std::vector<Cut>&  y_cuts = {}) const
    {
        const auto& x_res = this->fetchResult(phase_space, x_variable, region);
        const auto& y_res = this->fetchResult(phase_space, y_variable, region);

        auto sanitise = [&](std::string s) {
            for (auto& c : s)
                if (c == '.' || c == '/' || c == ' ')
                    c = '_';
            return s;
        };
        std::string name =
            "heatmap_" + sanitise(x_variable) + "_vs_" +
            sanitise(y_variable) + "_" +
            sanitise(region.empty() ? "default" : region);

        auto x_edges = x_res.binning_.getEdges();
        auto y_edges = y_res.binning_.getEdges();

        TH2F* hist = new TH2F(
            name.c_str(), name.c_str(),
            x_edges.size() - 1, x_edges.data(),
            y_edges.size() - 1, y_edges.data()
        );
        hist->GetXaxis()->SetTitle(x_res.binning_.getTexLabel().c_str());
        hist->GetYaxis()->SetTitle(y_res.binning_.getTexLabel().c_str());

        std::string filter = selection.str();
        
        
        
        
        
        bool has_filter =
            filter.find_first_not_of(" \t\n\r") != std::string::npos;

        for (auto& kv : loader_.getSampleFrames()) {
            auto& sample = kv.second;
            auto df = sample.nominal_node_;
            if (has_filter) {
                df = df.Filter(filter);
            }
            for (auto const& c : x_cuts) {
                std::string expr =
                    x_res.binning_.getVariable() +
                    (c.direction == CutDirection::GreaterThan ? ">" : "<") +
                    std::to_string(c.threshold);
                df = df.Filter(expr);
            }
            for (auto const& c : y_cuts) {
                std::string expr =
                    y_res.binning_.getVariable() +
                    (c.direction == CutDirection::GreaterThan ? ">" : "<") +
                    std::to_string(c.threshold);
                df = df.Filter(expr);
            }

            bool has_w = df.HasColumn("nominal_event_weight");
            auto xs = df.Take<double>(x_res.binning_.getVariable()).GetValue();
            auto ys = df.Take<double>(y_res.binning_.getVariable()).GetValue();
            std::vector<double> ws(xs.size(), 1.0);
            if (has_w) {
                ws = df.Take<double>("nominal_event_weight").GetValue();
            }
            for (size_t i = 0; i < xs.size(); ++i) {
                hist->Fill(xs[i], ys[i], ws[i]);
            }
        }

        HeatmapPlot plot(std::move(name), hist, output_directory_);
        plot.drawAndSave();
    }

    void generateEventDisplay(
        const EventIdentifier&  sample_event,
        const std::string&      sample_key) const
    {
        EventDisplay vis(
            loader_,
            image_size_,
            output_directory_
        );
        vis.visualiseEvent(sample_event, sample_key);
    }

    void generateRandomEventDisplays(
        const std::string& sample_key,
        const Selection&   sel,
        int                n_events,
        const std::string& pdf_name) const
    {
        generateRandomEventDisplays(sample_key, sel.str(), n_events, pdf_name);
    }

    void generateRandomEventDisplays(
        const std::string& sample_key,
        const std::string& region_filter,
        int                n_events,
        const std::string& pdf_name) const
    {
        auto& sample = loader_.getSampleFrames().at(SampleKey{sample_key});
        auto df = sample.nominal_node_;

        
        
        
        
        
        auto has_filter =
            region_filter.find_first_not_of(" \t\n\r") != std::string::npos;
        if (has_filter) {
            df = df.Filter(region_filter);
        }

        auto runs = df.Take<int>("run").GetValue();
        auto subs = df.Take<int>("sub").GetValue();
        auto evts = df.Take<int>("evt").GetValue();

        size_t total = runs.size();
        if (total == 0 || n_events <= 0) return;

        n_events = std::min<int>(n_events, static_cast<int>(total));

        std::vector<size_t> indices(total);
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), std::mt19937{std::random_device{}()});

        std::vector<EventIdentifier> events;
        events.reserve(n_events);
        for (int i = 0; i < n_events; ++i) {
            size_t idx = indices[i];
            events.emplace_back(runs[idx], subs[idx], evts[idx]);
        }

        EventDisplay vis(
            loader_,
            image_size_,
            output_directory_
        );
        vis.visualiseEvents(events, sample_key, pdf_name);
    }

private:
    const VariableResult& fetchResult(
        const RegionAnalysisMap& phase_space,
        const std::string&       variable,
        const std::string&       region) const
    {
        RegionKey   rkey{region};
        VariableKey vkey{variable};
        auto rit = phase_space.find(rkey);
        if (rit != phase_space.end()) {
            if (rit->second.hasFinalVariable(vkey)) {
                return rit->second.getFinalVariable(vkey);
            }
            log::fatal(
                "PlotCatalog::fetchResult",
                "Missing analysis result for variable",
                variable,
                "in region",
                region
            );
            throw std::runtime_error("Missing analysis result for variable");
        }
        log::fatal(
            "PlotCatalog::fetchResult",
            "Missing analysis result for region",
            region
        );
        throw std::runtime_error("Missing analysis result for region");
    }

    AnalysisDataLoader&         loader_;
    int                         image_size_;
    std::string                 output_directory_;
};

} 

#endif
