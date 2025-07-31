#ifndef BINNING_H
#define BINNING_H

#include <string>
#include <vector>
#include <stdexcept>
#include <cmath>
#include <numeric>
#include <algorithm>
#include "TString.h"
#include "Logger.h"

namespace AnalysisFramework {

class Binning {
public:
    const std::vector<double>& getBinEdges() const { return bin_edges_; }
    bool isLog() const { return is_log_; }
    int nBins() const noexcept {
        return bin_edges_.empty() ? 0 : static_cast<int>(bin_edges_.size()) - 1;
    }

    Binning(int n_bins, double low, double high, bool log = false) : is_log_(log) {
        if (n_bins <= 0) {
            log::error("Binning", "Number of bins must be positive.");
            throw std::invalid_argument("Number of bins must be positive.");
        }
        bin_edges_.resize(n_bins + 1);
        if (is_log_) {
            if (low <= 0) {
                log::error("Binning", "Log scale requires positive range.");
                throw std::invalid_argument("Log scale requires positive range.");
            }
            const double log_min = std::log10(low);
            const double step = (std::log10(high) - log_min) / n_bins;
            for (int i = 0; i <= n_bins; ++i) {
                bin_edges_[i] = std::pow(10, log_min + i * step);
            }
        } else {
            const double step = (high - low) / n_bins;
            for (int i = 0; i <= n_bins; ++i) {
                bin_edges_[i] = low + i * step;
            }
        }
    }

    Binning(const std::vector<double>& edges, bool log = false) : bin_edges_(edges), is_log_(log) {
        if (edges.size() < 2) {
            log::error("Binning", "Edges must contain at least two values.");
            throw std::invalid_argument("Edges must contain at least two values.");
        }
        if (!std::is_sorted(bin_edges_.cbegin(), bin_edges_.cend())) {
            log::error("Binning", "Edges must be sorted.");
            throw std::invalid_argument("Edges must be sorted.");
        }
    }

private:
    std::vector<double> bin_edges_;
    bool is_log_;
};

}
#endif