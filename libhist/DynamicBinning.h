#ifndef DYNAMIC_BINNING_H
#define DYNAMIC_BINNING_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

#include "BayesianBlocks.h"
#include "BinningDefinition.h"
#include "Logger.h"

namespace analysis {

enum class DynamicBinningStrategy { EqualWeight, UniformWidth, BayesianBlocks };

class DynamicBinning {
public:
  static BinningDefinition calculate(
      std::vector<ROOT::RDF::RNode> nodes,
      const BinningDefinition &original_bdef,
      const std::string &weight_col = "nominal_event_weight",
      bool include_oob_bins = false,
      DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight,
      double bin_resolution = 0.0) {
    if (nodes.empty()) {
      log::warn("DynamicBinning::calculate",
                "Cannot calculate bins: RNode vector is empty.");
      return original_bdef;
    }

    const std::string type_name = columnType(nodes, original_bdef);

    return dispatch(std::move(nodes), original_bdef, weight_col,
                    include_oob_bins, strategy, bin_resolution, type_name);
  }

private:
  using Handler = std::function<BinningDefinition(
      std::vector<ROOT::RDF::RNode>, const BinningDefinition &,
      const std::string &, bool, DynamicBinningStrategy, double)>;

  template <typename T> static Handler scalarHandler() {
    return
        [](std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &bdef,
           const std::string &weight_col, bool oob,
           DynamicBinningStrategy strat, double bin_res) {
          return calculateScalar<T>(std::move(nodes), bdef, weight_col, oob,
                                    strat, bin_res);
        };
  }

  template <typename T> static Handler vectorHandler() {
    return
        [](std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &bdef,
           const std::string &weight_col, bool oob,
           DynamicBinningStrategy strat, double bin_res) {
          return calculateVector<T>(std::move(nodes), bdef, weight_col, oob,
                                    strat, bin_res);
        };
  }

  static std::string columnType(std::vector<ROOT::RDF::RNode> &nodes,
                                const BinningDefinition &bdef) {
    const std::string &branch = bdef.getVariable();
    return nodes.front().GetColumnType(branch);
  }

  static BinningDefinition
  dispatch(std::vector<ROOT::RDF::RNode> nodes,
           const BinningDefinition &original_bdef,
           const std::string &weight_col, bool include_oob_bins,
           DynamicBinningStrategy strategy, double bin_resolution,
           const std::string &type_name) {
    static const std::unordered_map<std::string, Handler> kTypeDispatch = {
        {"double", scalarHandler<double>()},
        {"Float64_t", scalarHandler<double>()},
        {"Double_t", scalarHandler<double>()},

        {"float", scalarHandler<float>()},
        {"Float32_t", scalarHandler<float>()},
        {"Float_t", scalarHandler<float>()},

        {"int", scalarHandler<int>()},
        {"Int_t", scalarHandler<int>()},

        {"unsigned int", scalarHandler<unsigned int>()},
        {"UInt_t", scalarHandler<unsigned int>()},

        {"unsigned long", scalarHandler<unsigned long long>()},
        {"ULong64_t", scalarHandler<unsigned long long>()},
        {"unsigned long long", scalarHandler<unsigned long long>()},

        {"long", scalarHandler<long long>()},
        {"Long64_t", scalarHandler<long long>()},
        {"long long", scalarHandler<long long>()},

        {"vector<double>", vectorHandler<double>()},
        {"ROOT::RVec<double>", vectorHandler<double>()},
        {"ROOT::VecOps::RVec<double>", vectorHandler<double>()},

        {"vector<float>", vectorHandler<float>()},
        {"ROOT::RVec<float>", vectorHandler<float>()},
        {"ROOT::VecOps::RVec<float>", vectorHandler<float>()},

        {"vector<int>", vectorHandler<int>()},
        {"ROOT::RVec<int>", vectorHandler<int>()},
        {"ROOT::VecOps::RVec<int>", vectorHandler<int>()},

        {"vector<unsigned int>", vectorHandler<unsigned int>()},
        {"ROOT::RVec<unsigned int>", vectorHandler<unsigned int>()},
        {"ROOT::VecOps::RVec<unsigned int>", vectorHandler<unsigned int>()},

        {"vector<unsigned long>", vectorHandler<unsigned long long>()},
        {"vector<unsigned long long>", vectorHandler<unsigned long long>()},
        {"vector<ULong64_t>", vectorHandler<unsigned long long>()},
        {"ROOT::RVec<unsigned long>", vectorHandler<unsigned long long>()},
        {"ROOT::RVec<unsigned long long>", vectorHandler<unsigned long long>()},
        {"ROOT::VecOps::RVec<unsigned long>",
         vectorHandler<unsigned long long>()},
        {"ROOT::VecOps::RVec<unsigned long long>",
         vectorHandler<unsigned long long>()},

        {"vector<long long>", vectorHandler<long long>()},
        {"vector<Long64_t>", vectorHandler<long long>()},
        {"ROOT::RVec<long long>", vectorHandler<long long>()},
        {"ROOT::VecOps::RVec<long long>", vectorHandler<long long>()}};

    if (auto it = kTypeDispatch.find(type_name); it != kTypeDispatch.end()) {
      return it->second(std::move(nodes), original_bdef, weight_col,
                        include_oob_bins, strategy, bin_resolution);
    }

    for (const auto &entry : kTypeDispatch) {
      if (type_name.find(entry.first) != std::string::npos) {
        return entry.second(std::move(nodes), original_bdef, weight_col,
                            include_oob_bins, strategy, bin_resolution);
      }
    }

    log::fatal("DynamicBinning::dispatch",
               "Unsupported type for dynamic binning:", type_name);
    return original_bdef;
  }

  template <typename T>
  static BinningDefinition
  calculateScalar(std::vector<ROOT::RDF::RNode> nodes,
                  const BinningDefinition &original_bdef,
                  const std::string &weight_col, bool include_oob_bins,
                  DynamicBinningStrategy strategy, double bin_resolution) {
    std::vector<std::pair<double, double>> xw;
    xw.reserve(262144);
    double sumw = 0.0;
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
          }
        }
      } else {
        for (size_t i = 0; i < vals->size(); ++i) {
          double x = static_cast<double>((*vals)[i]);
          if (std::isfinite(x)) {
            xw.emplace_back(x, 1.0);
            sumw += 1.0;
          }
        }
      }
    }
    log::debug("DynamicBinning::calculateScalar", "Processed", xw.size(),
               "entries");
    return finaliseEdges(xw, sumw, original_bdef, include_oob_bins, strategy,
                         bin_resolution);
  }

  template <typename T>
  static BinningDefinition
  calculateVector(std::vector<ROOT::RDF::RNode> nodes,
                  const BinningDefinition &original_bdef,
                  const std::string &weight_col, bool include_oob_bins,
                  DynamicBinningStrategy strategy, double bin_resolution) {
    std::vector<std::pair<double, double>> xw;
    xw.reserve(262144);
    double sumw = 0.0;
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
            }
          }
        }
      }
    }
    log::debug("DynamicBinning::calculateVector", "Processed", xw.size(),
               "entries");
    return finaliseEdges(xw, sumw, original_bdef, include_oob_bins, strategy,
                         bin_resolution);
  }

  static void filterEntries(std::vector<std::pair<double, double>> &xw) {
    size_t before = xw.size();
    log::debug("DynamicBinning::filterEntries", "Starting with", before,
               "entries");

    const double float_low =
        static_cast<double>(std::numeric_limits<float>::lowest());
    const double float_high =
        static_cast<double>(std::numeric_limits<float>::max());
    const double double_low = std::numeric_limits<double>::lowest();
    const double double_high = std::numeric_limits<double>::max();

    xw.erase(std::remove_if(xw.begin(), xw.end(),
                            [&](const auto &p) {
                              double x = p.first;
                              double w = p.second;
                              bool invalid_x =
                                  !std::isfinite(x) || x == float_low ||
                                  x == float_high || x == double_low ||
                                  x == double_high;
                              bool invalid_w =
                                  !std::isfinite(w) || w <= 0.0;
                              return invalid_x || invalid_w;
                            }),
             xw.end());

    size_t removed = before - xw.size();

    if (removed > 0) {
      log::info("DynamicBinning::filterEntries", "Discarded", removed,
                "entries with non-finite values or non-positive weights");
    }
  }

  static std::vector<std::pair<double, double>>
  splitRangeEntries(const std::vector<std::pair<double, double>> &xw,
                    double domain_min, double domain_max) {
    std::vector<std::pair<double, double>> in_range;

    size_t n_underflow = 0;
    size_t n_overflow = 0;

    for (const auto &p : xw) {
      double x = p.first;
      if (x < domain_min) {
        ++n_underflow;
      } else if (x > domain_max) {
        ++n_overflow;
      } else {
        in_range.push_back(p);
      }
    }

    if (n_underflow > 0 || n_overflow > 0) {
      log::info("DynamicBinning::splitRangeEntries", "Found", n_underflow,
                "entries below domain and", n_overflow,
                "entries above domain; they will fill underflow/overflow bins");
    }

    return in_range;
  }

  static std::vector<double>
  applyStrategy(const std::vector<std::pair<double, double>> &in_range,
                double sumw, double xmin, double xmax, int target_bins,
                DynamicBinningStrategy strategy, double bin_resolution) {
    std::vector<double> edges;

    log::info("DynamicBinning::applyStrategy", "Starting with", in_range.size(),
              "entries spanning", xmin, "to", xmax, "strategy",
              static_cast<int>(strategy));

    auto add_uniform_edges = [&](int target_bins) {
      edges.reserve(static_cast<size_t>(target_bins) + 1);
      double bin_width = (xmax - xmin) / static_cast<double>(target_bins);
      edges.push_back(xmin);
      for (int k = 1; k < target_bins; ++k) {
        edges.push_back(xmin + k * bin_width);
      }
      edges.push_back(xmax);
    };

    if (strategy == DynamicBinningStrategy::BayesianBlocks) {
      std::vector<double> xs;
      std::vector<double> ws;
      xs.reserve(in_range.size());
      ws.reserve(in_range.size());
      auto quantise = [&](double x) {
        if (bin_resolution > 0.0) {
          return std::nearbyint((x - xmin) / bin_resolution) * bin_resolution +
                 xmin;
        }
        return x;
      };
      if (!in_range.empty()) {
        double current_x = quantise(in_range[0].first);
        double current_w = in_range[0].second;
        for (size_t i = 1; i < in_range.size(); ++i) {
          double qx = quantise(in_range[i].first);
          if (qx == current_x) {
            current_w += in_range[i].second;
          } else {
            xs.push_back(current_x);
            ws.push_back(current_w);
            current_x = qx;
            current_w = in_range[i].second;
          }
        }
        xs.push_back(current_x);
        ws.push_back(current_w);
      }
      if (xs.size() > 30000) {
        log::warn("DynamicBinning::applyStrategy",
                  "too many unique values for BayesianBlocks (", xs.size(),
                  ") falling back to EqualWeight");
        strategy = DynamicBinningStrategy::EqualWeight;
      } else {
        auto bb_edges = BayesianBlocks::blocks(xs, ws);
        edges.insert(edges.end(), bb_edges.begin(), bb_edges.end());
        log::info("DynamicBinning::applyStrategy", "BayesianBlocks produced",
                  edges.size() - 1, "bins");
        return edges;
      }
    }

    target_bins = std::max(1, target_bins);

    log::info("DynamicBinning::applyStrategy", "Target bins:", target_bins);

    if (strategy == DynamicBinningStrategy::UniformWidth) {
      add_uniform_edges(target_bins);
      log::info("DynamicBinning::applyStrategy", "UniformWidth produced",
                edges.size() - 1, "bins");
      return edges;
    }

    edges.reserve(static_cast<size_t>(target_bins) + 3);
    edges.push_back(xmin);

    if (target_bins > 1) {
      double W = sumw;
      double cum = 0.0;
      size_t idx = 0;
      for (int k = 1; k < target_bins; ++k) {
        double thresh =
            (static_cast<double>(k) / static_cast<double>(target_bins)) * W;
        while (idx < in_range.size() && cum + in_range[idx].second <= thresh) {
          cum += in_range[idx].second;
          ++idx;
        }
        if (idx < in_range.size()) {
          edges.push_back(in_range[idx].first);
        }
      }
    }

    edges.push_back(xmax);
    log::info("DynamicBinning::applyStrategy", "EqualWeight produced",
              edges.size() - 1, "bins");
    return edges;
  }

  static std::vector<double>
  finaliseEdgeList(std::vector<double> edges, bool include_oob_bins,
                   double domain_min, double domain_max) {
    log::info("DynamicBinning::finaliseEdgeList", "Starting with", edges.size(),
              "edges");

    edges.front() = domain_min;
    edges.back() = domain_max;

    if (include_oob_bins) {
      double first_width =
          edges.size() > 1 ? (edges[1] - edges[0]) : (domain_max - domain_min);
      double last_width =
          edges.size() > 1 ? (edges[edges.size() - 1] - edges[edges.size() - 2])
                           : (domain_max - domain_min);
      double underflow_width = 0.5 * first_width;
      double overflow_width = 0.5 * last_width;
      edges.insert(edges.begin(), domain_min - underflow_width);
      edges.push_back(domain_max + overflow_width);
      log::info("DynamicBinning::finaliseEdgeList",
                "Added underflow/overflow bins spanning",
                domain_min - underflow_width, "to",
                domain_max + overflow_width);
    }

    auto last = std::unique(edges.begin(), edges.end());
    edges.erase(last, edges.end());

    if (edges.size() < 2) {
      edges = {domain_min, domain_max};
    }

    for (size_t i = 1; i < edges.size(); ++i) {
      if (!(edges[i] > edges[i - 1])) {
        edges[i] = std::nextafter(edges[i - 1],
                                  std::numeric_limits<double>::infinity());
      }
    }

    log::info("DynamicBinning::finaliseEdgeList", "Finished with",
              edges.size() - 1, "bins");

    return edges;
  }

  static BinningDefinition
  finaliseEdges(std::vector<std::pair<double, double>> &xw, double sumw,
                const BinningDefinition &original_bdef, bool include_oob_bins,
                DynamicBinningStrategy strategy, double bin_resolution) {
    const auto &domain_edges = original_bdef.getEdges();
    double domain_min = domain_edges.front();
    double domain_max = domain_edges.back();

    filterEntries(xw);

    auto in_range = splitRangeEntries(xw, domain_min, domain_max);

    log::info("DynamicBinning::finaliseEdges",
              "In-range entries:", in_range.size());

    if (!in_range.empty()) {
      auto mm = std::minmax_element(
          in_range.begin(), in_range.end(),
          [](const auto &a, const auto &b) { return a.first < b.first; });
      if (!std::isfinite(domain_min))
        domain_min = mm.first->first;
      if (!std::isfinite(domain_max))
        domain_max = mm.second->first;
    } else {
      if (!std::isfinite(domain_min))
        domain_min = 0.0;
      if (!std::isfinite(domain_max))
        domain_max = 1.0;
    }

    if (in_range.size() < 2) {
      return BinningDefinition({domain_min, domain_max},
                               original_bdef.getVariable(),
                               original_bdef.getTexLabel(), {},
                               original_bdef.getStratifierKey().str());
    }

    std::sort(in_range.begin(), in_range.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    sumw = 0.0;
    for (const auto &p : in_range) {
      sumw += p.second;
    }

    if (sumw <= 0.0) {
      return BinningDefinition({domain_min, domain_max},
                               original_bdef.getVariable(),
                               original_bdef.getTexLabel(), {},
                               original_bdef.getStratifierKey().str());
    }

    double xmin = domain_min;
    double xmax = domain_max;

    log::info("DynamicBinning::finaliseEdges", "Using fixed data range for",
              original_bdef.getVariable(), ":", xmin, "to", xmax);

    int target_bins = original_bdef.getBinNumber();
    auto edges =
        applyStrategy(in_range, sumw, xmin, xmax, target_bins, strategy,
                      bin_resolution);

    log::info("DynamicBinning::finaliseEdges", "applyStrategy returned",
              edges.size(), "edges");

    edges = finaliseEdgeList(std::move(edges), include_oob_bins, domain_min,
                             domain_max);

    log::info("DynamicBinning::finaliseEdges", "finaliseEdgeList returned",
              edges.size(), "edges");

    return BinningDefinition(edges, original_bdef.getVariable(),
                             original_bdef.getTexLabel(), {},
                             original_bdef.getStratifierKey().str());
  }
};

} // namespace analysis

#endif
