#ifndef DYNAMIC_BINNING_CALCULATOR_H
#define DYNAMIC_BINNING_CALCULATOR_H

#include <ROOT/RDataFrame.hxx>
#include <ROOT/RVec.hxx>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "AnalysisLogger.h"
#include "BinningDefinition.h"

namespace analysis {

class DynamicBinningCalculator {
public:
  static BinningDefinition
  calculate(std::vector<ROOT::RDF::RNode> nodes,
            const BinningDefinition &original_bdef,
            const std::string &weight_col = "nominal_event_weight",
            double min_neff_per_bin = 50.0) {
    if (nodes.empty()) {
      log::warn("DynamicBinningCalculator",
                "Cannot calculate bins: RNode vector is empty.");
      return original_bdef;
    }

    const std::string &branch = original_bdef.getVariable();
    std::string typeName = nodes.front().GetColumnType(branch);

    if (typeName == "double" || typeName == "Float64_t" ||
        typeName == "Double_t") {
      return calculate_scalar<double>(std::move(nodes), original_bdef,
                                      weight_col, min_neff_per_bin);
    } else if (typeName == "float" || typeName == "Float32_t" ||
               typeName == "Float_t") {
      return calculate_scalar<float>(std::move(nodes), original_bdef,
                                     weight_col, min_neff_per_bin);
    } else if (typeName == "int" || typeName == "Int_t") {
      return calculate_scalar<int>(std::move(nodes), original_bdef, weight_col,
                                   min_neff_per_bin);
    } else if (typeName == "unsigned int" || typeName == "UInt_t") {
      return calculate_scalar<unsigned int>(std::move(nodes), original_bdef,
                                            weight_col, min_neff_per_bin);
    } else if (typeName == "long" || typeName == "Long64_t" ||
               typeName == "long long") {
      return calculate_scalar<long long>(std::move(nodes), original_bdef,
                                         weight_col, min_neff_per_bin);
    } else if (typeName.find("ROOT::VecOps::RVec<double>") !=
                   std::string::npos ||
               typeName.find("ROOT::RVec<double>") != std::string::npos ||
               typeName.find("vector<double>") != std::string::npos) {
      return calculate_vector<double>(std::move(nodes), original_bdef,
                                      weight_col, min_neff_per_bin);
    } else if (typeName.find("ROOT::VecOps::RVec<float>") !=
                   std::string::npos ||
               typeName.find("ROOT::RVec<float>") != std::string::npos ||
               typeName.find("vector<float>") != std::string::npos) {
      return calculate_vector<float>(std::move(nodes), original_bdef,
                                     weight_col, min_neff_per_bin);
    } else if (typeName.find("ROOT::VecOps::RVec<int>") != std::string::npos ||
               typeName.find("ROOT::RVec<int>") != std::string::npos ||
               typeName.find("vector<int>") != std::string::npos) {
      return calculate_vector<int>(std::move(nodes), original_bdef, weight_col,
                                   min_neff_per_bin);
    } else if (typeName.find("ROOT::VecOps::RVec<unsigned int>") !=
                   std::string::npos ||
               typeName.find("ROOT::RVec<unsigned int>") != std::string::npos ||
               typeName.find("vector<unsigned int>") != std::string::npos) {
      return calculate_vector<unsigned int>(std::move(nodes), original_bdef,
                                            weight_col, min_neff_per_bin);
    } else if (typeName.find("ROOT::VecOps::RVec<long long>") !=
                   std::string::npos ||
               typeName.find("ROOT::RVec<long long>") != std::string::npos ||
               typeName.find("vector<long long>") != std::string::npos ||
               typeName.find("vector<Long64_t>") != std::string::npos) {
      return calculate_vector<long long>(std::move(nodes), original_bdef,
                                         weight_col, min_neff_per_bin);
    } else {
      log::fatal("DynamicBinningCalculator",
                 "Unsupported type for dynamic binning:", typeName);
      return original_bdef;
    }
  }

private:
  template <typename T>
  static BinningDefinition
  calculate_scalar(std::vector<ROOT::RDF::RNode> nodes,
                   const BinningDefinition &original_bdef,
                   const std::string &weight_col, double min_neff_per_bin) {
    std::vector<std::pair<double, double>> xw;
    xw.reserve(262144);
    double sumw = 0.0;
    double sumw2 = 0.0;
    const std::string &branch = original_bdef.getVariable();

    for (auto &n : nodes) {
      bool has_w = n.HasColumn(weight_col);
      auto vals = n.template Take<T>(branch);
      if (has_w) {
        auto ws = n.template Take<double>(weight_col);
        for (size_t i = 0; i < vals->size(); ++i) {
          double x = static_cast<double>((*vals)[i]);
          double w = (*ws)[i];
          if (std::isfinite(x) && std::isfinite(w) && w > 0.0) {
            xw.emplace_back(x, w);
            sumw += w;
            sumw2 += w * w;
          }
        }
      } else {
        for (size_t i = 0; i < vals->size(); ++i) {
          double x = static_cast<double>((*vals)[i]);
          if (std::isfinite(x)) {
            xw.emplace_back(x, 1.0);
            sumw += 1.0;
            sumw2 += 1.0;
          }
        }
      }
    }
    return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin);
  }

  template <typename T>
  static BinningDefinition
  calculate_vector(std::vector<ROOT::RDF::RNode> nodes,
                   const BinningDefinition &original_bdef,
                   const std::string &weight_col, double min_neff_per_bin) {
    std::vector<std::pair<double, double>> xw;
    xw.reserve(262144);
    double sumw = 0.0;
    double sumw2 = 0.0;
    const std::string &branch = original_bdef.getVariable();

    for (auto &n : nodes) {
      bool has_w = n.HasColumn(weight_col);
      auto vecs = n.template Take<ROOT::RVec<T>>(branch);
      if (has_w) {
        auto ws = n.template Take<double>(weight_col);
        for (size_t i = 0; i < vecs->size(); ++i) {
          double w = (*ws)[i];
          if (!std::isfinite(w) || w <= 0.0)
            continue;
          for (auto val : (*vecs)[i]) {
            double x = static_cast<double>(val);
            if (std::isfinite(x)) {
              xw.emplace_back(x, w);
              sumw += w;
              sumw2 += w * w;
            }
          }
        }
      } else {
        for (size_t i = 0; i < vecs->size(); ++i) {
          for (auto val : (*vecs)[i]) {
            double x = static_cast<double>(val);
            if (std::isfinite(x)) {
              xw.emplace_back(x, 1.0);
              sumw += 1.0;
              sumw2 += 1.0;
            }
          }
        }
      }
    }
    return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin);
  }

  static BinningDefinition
  finalize_edges(std::vector<std::pair<double, double>> &xw, double sumw,
                 double sumw2, const BinningDefinition &original_bdef,
                 double min_neff_per_bin) {
    if (xw.size() < 2 || sumw <= 0.0) {
      return BinningDefinition({0.0, 1.0}, original_bdef.getVariable(),
                               original_bdef.getTexLabel(), {},
                               original_bdef.getStratifierKey().str());
    }

    std::sort(xw.begin(), xw.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    double neff_total =
        (sumw * sumw) / std::max(sumw2, std::numeric_limits<double>::min());
    int target_bins = static_cast<int>(
        std::floor(neff_total / std::max(min_neff_per_bin, 1.0)));
    if (target_bins < 1)
      target_bins = 1;

    std::vector<double> edges;
    edges.reserve(static_cast<size_t>(target_bins) + 1);
    edges.push_back(xw.front().first);

    if (target_bins > 1) {
      double W = sumw;
      double cum = 0.0;
      size_t idx = 0;
      for (int k = 1; k < target_bins; ++k) {
        double thresh =
            (static_cast<double>(k) / static_cast<double>(target_bins)) * W;
        while (idx < xw.size() && cum + xw[idx].second < thresh) {
          cum += xw[idx].second;
          ++idx;
        }
        if (idx < xw.size()) {
          edges.push_back(xw[idx].first);
        }
      }
    }

    edges.push_back(xw.back().first);

    auto last = std::unique(edges.begin(), edges.end());
    edges.erase(last, edges.end());

    if (edges.size() < 2) {
      edges = {xw.front().first, xw.back().first};
    }

    for (size_t i = 1; i < edges.size(); ++i) {
      if (!(edges[i] > edges[i - 1])) {
        edges[i] = std::nextafter(edges[i - 1],
                                  std::numeric_limits<double>::infinity());
      }
    }

    return BinningDefinition(edges, original_bdef.getVariable(),
                             original_bdef.getTexLabel(), {},
                             original_bdef.getStratifierKey().str());
  }
};

}

#endif
