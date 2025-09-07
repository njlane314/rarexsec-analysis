#ifndef DYNAMIC_BINNING_H
#define DYNAMIC_BINNING_H

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
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
    static BinningDefinition calculate(std::vector<ROOT::RDF::RNode> nodes,
                                       const BinningDefinition &original_bdef,
                                       const std::string &weight_col = "nominal_event_weight",
                                       double min_neff_per_bin = 400.0,
                                       bool include_oob_bins = false,
                                       DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight) {
        if (nodes.empty()) {
            log::warn("DynamicBinning::calculate",
                      "Cannot calculate bins: RNode vector is empty.");
            return original_bdef;
        }

        const std::string type_name = columnType(nodes, original_bdef);

        return dispatch(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                        include_oob_bins, strategy, type_name);
    }

private:
    using Handler = std::function<BinningDefinition(std::vector<ROOT::RDF::RNode>,
                                                    const BinningDefinition &,
                                                    const std::string &, double,
                                                    bool, DynamicBinningStrategy)>;

    template <typename T>
    static Handler scalarHandler() {
        return [](std::vector<ROOT::RDF::RNode> nodes,
                  const BinningDefinition &bdef,
                  const std::string &weight_col,
                  double min_neff,
                  bool oob,
                  DynamicBinningStrategy strat) {
            return calculateScalar<T>(std::move(nodes), bdef, weight_col,
                                      min_neff, oob, strat);
        };
    }

    template <typename T>
    static Handler vectorHandler() {
        return [](std::vector<ROOT::RDF::RNode> nodes,
                  const BinningDefinition &bdef,
                  const std::string &weight_col,
                  double min_neff,
                  bool oob,
                  DynamicBinningStrategy strat) {
            return calculateVector<T>(std::move(nodes), bdef, weight_col,
                                      min_neff, oob, strat);
        };
    }

    static std::string columnType(std::vector<ROOT::RDF::RNode> &nodes,
                                  const BinningDefinition &bdef) {
        const std::string &branch = bdef.getVariable();
        return nodes.front().GetColumnType(branch);
    }

    static BinningDefinition dispatch(std::vector<ROOT::RDF::RNode> nodes,
                                      const BinningDefinition &original_bdef,
                                      const std::string &weight_col,
                                      double min_neff_per_bin,
                                      bool include_oob_bins,
                                      DynamicBinningStrategy strategy,
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
            {"ROOT::VecOps::RVec<unsigned long>", vectorHandler<unsigned long long>()},
            {"ROOT::VecOps::RVec<unsigned long long>", vectorHandler<unsigned long long>()},

            {"vector<long long>", vectorHandler<long long>()},
            {"vector<Long64_t>", vectorHandler<long long>()},
            {"ROOT::RVec<long long>", vectorHandler<long long>()},
            {"ROOT::VecOps::RVec<long long>", vectorHandler<long long>()}
        };

        if (auto it = kTypeDispatch.find(type_name); it != kTypeDispatch.end()) {
            return it->second(std::move(nodes), original_bdef, weight_col,
                              min_neff_per_bin, include_oob_bins, strategy);
        }

        for (const auto &entry : kTypeDispatch) {
            if (type_name.find(entry.first) != std::string::npos) {
                return entry.second(std::move(nodes), original_bdef, weight_col,
                                    min_neff_per_bin, include_oob_bins, strategy);
            }
        }

        log::fatal("DynamicBinning::dispatch",
                   "Unsupported type for dynamic binning:", type_name);
        return original_bdef;
    }

    template <typename T>
    static BinningDefinition calculateScalar(std::vector<ROOT::RDF::RNode> nodes,
                                             const BinningDefinition &original_bdef,
                                             const std::string &weight_col,
                                             double min_neff_per_bin,
                                             bool include_oob_bins,
                                             DynamicBinningStrategy strategy) {
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
        return finaliseEdges(xw, sumw, sumw2, original_bdef,
                             min_neff_per_bin, include_oob_bins, strategy);
    }

    template <typename T>
    static BinningDefinition calculateVector(std::vector<ROOT::RDF::RNode> nodes,
                                             const BinningDefinition &original_bdef,
                                             const std::string &weight_col,
                                             double min_neff_per_bin,
                                             bool include_oob_bins,
                                             DynamicBinningStrategy strategy) {
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
        return finaliseEdges(xw, sumw, sumw2, original_bdef,
                             min_neff_per_bin, include_oob_bins, strategy);
    }

    static void filterEntries(std::vector<std::pair<double, double>> &xw) {
        size_t before = xw.size();

        xw.erase(std::remove_if(xw.begin(), xw.end(), [&](const auto &p) {
                                    double x = p.first;
                                    double w = p.second;
                                    return !std::isfinite(x) || !std::isfinite(w) || w <= 0.0;
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

    static std::vector<double> applyStrategy(
        const std::vector<std::pair<double, double>> &in_range, double sumw,
        double sumw2, double xmin, double xmax, double min_neff_per_bin,
        DynamicBinningStrategy strategy) {
        std::vector<double> edges;

        double neff_total =
            (sumw * sumw) / std::max(sumw2, std::numeric_limits<double>::min());

        auto add_uniform_edges = [&](int target_bins) {
            edges.reserve(static_cast<size_t>(target_bins) + 1);
            double bin_width = (xmax - xmin) / static_cast<double>(target_bins);
            edges.push_back(xmin);
            for (int k = 1; k < target_bins; ++k) {
                edges.push_back(xmin + k * bin_width);
            }
            edges.push_back(xmax);
        };

        switch (strategy) {
        case DynamicBinningStrategy::BayesianBlocks: {
            std::map<double, double> hist;
            for (const auto &p : in_range)
                hist[p.first] += p.second;
            std::vector<double> xs;
            std::vector<double> ws;
            xs.reserve(hist.size());
            ws.reserve(hist.size());
            for (auto &kv : hist) {
                xs.push_back(kv.first);
                ws.push_back(kv.second);
            }
            auto bb_edges = BayesianBlocks::blocks(xs, ws);
            edges.insert(edges.end(), bb_edges.begin(), bb_edges.end());
            break;
        }
        case DynamicBinningStrategy::UniformWidth: {
            int target_bins =
                std::max(1, static_cast<int>(
                                                std::floor(neff_total /
                                                           std::max(min_neff_per_bin, 1.0))));
            add_uniform_edges(target_bins);
            break;
        }
        case DynamicBinningStrategy::EqualWeight:
        default: {
            int target_bins =
                std::max(1, static_cast<int>(
                                                std::floor(neff_total /
                                                           std::max(min_neff_per_bin, 1.0))));

            edges.reserve(static_cast<size_t>(target_bins) + 3);
            edges.push_back(xmin);

            if (target_bins > 1) {
                double W = sumw;
                double cum = 0.0;
                size_t idx = 0;
                for (int k = 1; k < target_bins; ++k) {
                    double thresh =
                        (static_cast<double>(k) / static_cast<double>(target_bins)) *
                        W;
                    while (idx < in_range.size() &&
                           cum + in_range[idx].second <= thresh) {
                        cum += in_range[idx].second;
                        ++idx;
                    }
                    if (idx < in_range.size()) {
                        edges.push_back(in_range[idx].first);
                    }
                }
            }

            edges.push_back(xmax);
            break;
        }
        }

        return edges;
    }

    static std::vector<double>
    finaliseEdgeList(std::vector<double> edges,
                     const std::vector<std::pair<double, double>> &in_range,
                     double min_neff_per_bin, bool include_oob_bins,
                     double domain_min, double domain_max) {
        edges.front() = domain_min;
        edges.back() = domain_max;

        if (min_neff_per_bin > 0.0 && edges.size() > 2) {
            bool merged = true;
            while (merged && edges.size() > 2) {
                merged = false;
                size_t nbins = edges.size() - 1;
                std::vector<double> sw(nbins, 0.0), sw2(nbins, 0.0);
                size_t bin = 0;
                for (const auto &p : in_range) {
                    double x = p.first;
                    double w = p.second;
                    while (bin < nbins - 1 && x >= edges[bin + 1]) {
                        ++bin;
                    }
                    sw[bin] += w;
                    sw2[bin] += w * w;
                }
                for (size_t i = 0; i < nbins; ++i) {
                    double neff = (sw[i] * sw[i]) /
                                  std::max(sw2[i],
                                           std::numeric_limits<double>::min());
                    if (neff < min_neff_per_bin) {
                        if (i < nbins - 1) {
                            edges.erase(edges.begin() + i + 1);
                        } else {
                            edges.erase(edges.begin() + i);
                        }
                        merged = true;
                        break;
                    }
                }
            }
        }

        if (include_oob_bins) {
            double first_width =
                edges.size() > 1 ? (edges[1] - edges[0])
                                 : (domain_max - domain_min);
            double last_width = edges.size() > 1
                                    ? (edges[edges.size() - 1] -
                                       edges[edges.size() - 2])
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

        return edges;
    }

    static BinningDefinition finaliseEdges(
        std::vector<std::pair<double, double>> &xw, double sumw,
        double sumw2, const BinningDefinition &original_bdef,
        double min_neff_per_bin, bool include_oob_bins,
        DynamicBinningStrategy strategy) {
        const auto &domain_edges = original_bdef.getEdges();
        double domain_min = domain_edges.front();
        double domain_max = domain_edges.back();

        filterEntries(xw);

        auto in_range = splitRangeEntries(xw, domain_min, domain_max);

        if (in_range.size() < 2) {
            return BinningDefinition({domain_min, domain_max},
                                     original_bdef.getVariable(),
                                     original_bdef.getTexLabel(), {},
                                     original_bdef.getStratifierKey().str());
        }

        std::sort(in_range.begin(), in_range.end(),
                  [](const auto &a, const auto &b) { return a.first < b.first; });

        sumw = 0.0;
        sumw2 = 0.0;
        for (const auto &p : in_range) {
            sumw += p.second;
            sumw2 += p.second * p.second;
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

        auto edges = applyStrategy(in_range, sumw, sumw2, xmin, xmax,
                                   min_neff_per_bin, strategy);

        edges = finaliseEdgeList(std::move(edges), in_range, min_neff_per_bin,
                                 include_oob_bins, domain_min, domain_max);

        return BinningDefinition(edges, original_bdef.getVariable(),
                                 original_bdef.getTexLabel(), {},
                                 original_bdef.getStratifierKey().str());
    }
};

} // namespace analysis

#endif

