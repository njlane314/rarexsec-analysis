#ifndef BINNING_H
#define BINNING_H

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <algorithm>
#include <utility>

#include "TString.h"
#include "TAxis.h"
#include "TMath.h"
#include "Selection.h" 

namespace AnalysisFramework {

class Binning {
public:
    struct Parameters {
        TString variable;
        TString label = "";
        TString variable_tex = "";
        TString variable_tex_short = "";
        int number_of_bins = 0; 
        std::pair<double, double> range = {0.0, 0.0};
        std::vector<double> bin_edges;
        bool is_log = false;
        std::vector<TString> selection_keys;
        TString selection_key = "";
        TString selection_tex = "";
        TString selection_tex_short = "";
        bool is_particle_level = false;
    };

    TString variable;
    std::vector<double> bin_edges;
    TString label;
    TString variable_tex;
    TString variable_tex_short;
    bool is_log;
    TString selection_query;
    std::vector<TString> selection_keys; 
    TString selection_tex;
    TString selection_tex_short;
    bool is_particle_level; 

    inline explicit Binning(const Parameters& p) {
        if (p.variable.IsNull() || p.variable.IsWhitespace()) {
            throw std::invalid_argument("Binning::Parameters::variable must be set.");
        }
        if (p.number_of_bins > 0 && !p.bin_edges.empty()) { 
            throw std::invalid_argument("Provide either number_of_bins/range (for uniform) or bin_edges (for custom), not both.");
        }
        if (p.number_of_bins <= 0 && p.bin_edges.empty()) { 
            throw std::invalid_argument("Either number_of_bins or bin_edges must be specified.");
        }

        variable = p.variable;
        label = p.label.IsNull() ? p.variable : p.label;
        variable_tex = p.variable_tex.IsNull() ? p.variable : p.variable_tex;
        variable_tex_short = p.variable_tex_short;
        is_log = p.is_log;
        is_particle_level = p.is_particle_level;
        
        if (!p.selection_keys.empty()) {
            selection_keys = p.selection_keys;
            selection_query = AnalysisFramework::Selection::getSelectionQuery(p.selection_keys);
            selection_tex = p.selection_tex;
            selection_tex_short = p.selection_tex_short;
        }

        if (p.number_of_bins > 0) { 
            const auto [minVal, maxVal] = p.range;
            bin_edges.resize(p.number_of_bins + 1); 
            if (is_log) {
                if (minVal <= 0) throw std::invalid_argument("Log scale requires positive range limits.");
                const double logMin = std::log10(minVal);
                const double logMax = std::log10(maxVal);
                const double step = (logMax - logMin) / p.number_of_bins; 
                for (int i = 0; i <= p.number_of_bins; ++i) bin_edges[i] = std::pow(10, logMin + i * step); 
            } else {
                const double step = (maxVal - minVal) / p.number_of_bins; 
                for (int i = 0; i <= p.number_of_bins; ++i) bin_edges[i] = minVal + i * step; 
            }
        } else {
            bin_edges = p.bin_edges;
            if (bin_edges.size() < 2) {
                throw std::invalid_argument("Custom bin_edges must contain at least two values.");
            }
            if (!std::is_sorted(bin_edges.cbegin(), bin_edges.cend())) {
                throw std::invalid_argument("Custom bin_edges must be sorted.");
            }
        }
    }

    Binning() = default; 

    inline int nBins() const noexcept {
        return bin_edges.empty() ? 0 : static_cast<int>(bin_edges.size()) - 1;
    }

    inline std::vector<double> binCenters() const {
        if (nBins() == 0) return {};
        std::vector<double> centers(nBins());
        for (int i = 0; i < nBins(); ++i) {
            if (is_log) {
                centers[i] = std::sqrt(bin_edges[i] * bin_edges[i + 1]);
            } else {
                centers[i] = (bin_edges[i] + bin_edges[i + 1]) / 2.0;
            }
        }
        return centers;
    }

    inline bool isCompatible(const Binning& other) const noexcept {
        return variable == other.variable &&
               bin_edges == other.bin_edges &&
               is_log == other.is_log;
    }
};

}

#endif