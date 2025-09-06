#ifndef ADAPTIVE_BINNING_CALCULATOR_H
#define ADAPTIVE_BINNING_CALCULATOR_H

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

#include "AnalysisLogger.h"
#include "BayesianBlocks.h"
#include "BinningDefinition.h"

namespace analysis {

enum class DynamicBinningStrategy { EqualWeight, FreedmanDiaconis, Scott, Sturges, Rice, Sqrt, BayesianBlocks };

class DynamicBinning {
  public:
    static BinningDefinition calculate(std::vector<ROOT::RDF::RNode> nodes, const BinningDefinition &original_bdef,
                                       const std::string &weight_col = "nominal_event_weight",
                                       double min_neff_per_bin = 400.0, bool include_out_of_range_bins = false,
                                       DynamicBinningStrategy strategy = DynamicBinningStrategy::EqualWeight) {
        if (nodes.empty()) {
            log::warn("DynamicBinning::calculate", "Cannot calculate bins: RNode vector is empty.");
            return original_bdef;
        }

        const std::string &branch = original_bdef.getVariable();
        std::string typeName = nodes.front().GetColumnType(branch);

        auto match = [&](std::initializer_list<std::string> names) {
            return std::find(names.begin(), names.end(), typeName) != names.end();
        };

        auto contains = [&](std::initializer_list<std::string> names) {
            return std::any_of(names.begin(), names.end(),
                               [&](const std::string &s) { return typeName.find(s) != std::string::npos; });
        };

        if (match({"double", "Float64_t", "Double_t"})) {
            return calculate_scalar<double>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                            include_out_of_range_bins, strategy);
        } else if (match({"float", "Float32_t", "Float_t"})) {
            return calculate_scalar<float>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                           include_out_of_range_bins, strategy);
        } else if (match({"int", "Int_t"})) {
            return calculate_scalar<int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                         include_out_of_range_bins, strategy);
        } else if (match({"unsigned int", "UInt_t"})) {
            return calculate_scalar<unsigned int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                                  include_out_of_range_bins, strategy);
        } else if (match({"unsigned long", "ULong64_t", "unsigned long long"})) {
            return calculate_scalar<unsigned long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                                        include_out_of_range_bins, strategy);
        } else if (match({"long", "Long64_t", "long long"})) {
            return calculate_scalar<long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                               include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<double>", "ROOT::RVec<double>", "vector<double>"})) {
            return calculate_vector<double>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                            include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<float>", "ROOT::RVec<float>", "vector<float>"})) {
            return calculate_vector<float>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                           include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<int>", "ROOT::RVec<int>", "vector<int>"})) {
            return calculate_vector<int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                         include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<unsigned int>", "ROOT::RVec<unsigned int>", "vector<unsigned int>"})) {
            return calculate_vector<unsigned int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                                  include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<unsigned long>", "ROOT::RVec<unsigned long>", "vector<unsigned long>",
                             "vector<ULong64_t>", "ROOT::VecOps::RVec<unsigned long long>",
                             "ROOT::RVec<unsigned long long>", "vector<unsigned long long>"})) {
            return calculate_vector<unsigned long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                                        include_out_of_range_bins, strategy);
        } else if (contains({"ROOT::VecOps::RVec<long long>", "ROOT::RVec<long long>", "vector<long long>",
                             "vector<Long64_t>"})) {
            return calculate_vector<long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin,
                                               include_out_of_range_bins, strategy);
        } else {
            log::fatal("DynamicBinning::calculate", "Unsupported type for dynamic binning:", typeName);
            return original_bdef;
        }
    }

  private:
    template <typename T>
    static BinningDefinition calculate_scalar(std::vector<ROOT::RDF::RNode> nodes,
                                              const BinningDefinition &original_bdef, const std::string &weight_col,
                                              double min_neff_per_bin, bool include_out_of_range_bins,
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
        return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin, include_out_of_range_bins, strategy);
    }

    template <typename T>
    static BinningDefinition calculate_vector(std::vector<ROOT::RDF::RNode> nodes,
                                              const BinningDefinition &original_bdef, const std::string &weight_col,
                                              double min_neff_per_bin, bool include_out_of_range_bins,
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
        return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin, include_out_of_range_bins, strategy);
    }

    static BinningDefinition finalize_edges(std::vector<std::pair<double, double>> &xw, double sumw, double sumw2,
                                            const BinningDefinition &original_bdef, double min_neff_per_bin,
                                            bool include_out_of_range_bins, DynamicBinningStrategy strategy) {
        const auto &domain_edges = original_bdef.getEdges();
        double domain_min = domain_edges.front();
        double domain_max = domain_edges.back();

        size_t before_filter = xw.size();
        xw.erase(std::remove_if(xw.begin(), xw.end(),
                                [&](const auto &p) {
                                    double x = p.first;
                                    double w = p.second;
                                    return !std::isfinite(x) || !std::isfinite(w) || w <= 0.0;
                                }),
                 xw.end());
        size_t removed_invalid = before_filter - xw.size();
        if (removed_invalid > 0) {
            log::info("DynamicBinning::finalize_edges", "Discarded", removed_invalid,
                      "entries with non-finite values or non-positive weights");
        }

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
            log::info("DynamicBinning::finalize_edges", "Found", n_underflow, "entries below domain and", n_overflow,
                      "entries above domain; they will fill underflow/overflow bins");
        }

        if (in_range.size() < 2) {
            return BinningDefinition({domain_min, domain_max}, original_bdef.getVariable(), original_bdef.getTexLabel(),
                                     {}, original_bdef.getStratifierKey().str());
        }

        std::sort(in_range.begin(), in_range.end(), [](const auto &a, const auto &b) { return a.first < b.first; });

        sumw = 0.0;
        sumw2 = 0.0;
        for (const auto &p : in_range) {
            sumw += p.second;
            sumw2 += p.second * p.second;
        }

        if (sumw <= 0.0) {
            return BinningDefinition({domain_min, domain_max}, original_bdef.getVariable(), original_bdef.getTexLabel(),
                                     {}, original_bdef.getStratifierKey().str());
        }

        double xmin = domain_min;
        double xmax = domain_max;
        log::info("DynamicBinning::finalize_edges", "Using fixed data range for", original_bdef.getVariable(), ":",
                  xmin, "to", xmax);

        std::vector<double> edges;

        double neff_total = (sumw * sumw) / std::max(sumw2, std::numeric_limits<double>::min());

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
        case DynamicBinningStrategy::FreedmanDiaconis: {
            auto quant = [&](double q) {
                double target = q * sumw;
                double cum = 0.0;
                for (const auto &p : in_range) {
                    cum += p.second;
                    if (cum >= target)
                        return p.first;
                }
                return in_range.back().first;
            };

            double q1 = quant(0.25);
            double q3 = quant(0.75);
            double iqr = q3 - q1;
            if (!(iqr > 0.0)) {
                iqr = xmax - xmin;
            }
            size_t n = in_range.size();
            double bin_width = 2.0 * iqr * std::pow(static_cast<double>(n), -1.0 / 3.0);
            if (!(bin_width > 0.0)) {
                bin_width = xmax - xmin;
            }
            int target_bins = std::max(1, static_cast<int>(std::ceil((xmax - xmin) / bin_width)));
            add_uniform_edges(target_bins);
            break;
        }
        case DynamicBinningStrategy::Scott: {
            double sumwx = 0.0;
            for (const auto &p : in_range) {
                sumwx += p.first * p.second;
            }
            double mean = sumwx / sumw;
            double swvar = 0.0;
            for (const auto &p : in_range) {
                double diff = p.first - mean;
                swvar += p.second * diff * diff;
            }
            double sigma = std::sqrt(swvar / sumw);
            double bin_width = 3.5 * sigma * std::pow(neff_total, -1.0 / 3.0);
            if (!(bin_width > 0.0)) {
                bin_width = xmax - xmin;
            }
            int target_bins = std::max(1, static_cast<int>(std::ceil((xmax - xmin) / bin_width)));
            add_uniform_edges(target_bins);
            break;
        }
        case DynamicBinningStrategy::Sturges: {
            int target_bins = std::max(1, static_cast<int>(std::ceil(std::log2(neff_total) + 1.0)));
            add_uniform_edges(target_bins);
            break;
        }
        case DynamicBinningStrategy::Rice: {
            int target_bins = std::max(1, static_cast<int>(std::ceil(2.0 * std::cbrt(neff_total))));
            add_uniform_edges(target_bins);
            break;
        }
        case DynamicBinningStrategy::Sqrt: {
            int target_bins = std::max(1, static_cast<int>(std::ceil(std::sqrt(neff_total))));
            add_uniform_edges(target_bins);
            break;
        }
        default: {
            int target_bins = std::max(1, static_cast<int>(std::floor(neff_total / std::max(min_neff_per_bin, 1.0))));

            edges.reserve(static_cast<size_t>(target_bins) + 3);
            edges.push_back(xmin);

            if (target_bins > 1) {
                double W = sumw;
                double cum = 0.0;
                size_t idx = 0;
                for (int k = 1; k < target_bins; ++k) {
                    double thresh = (static_cast<double>(k) / static_cast<double>(target_bins)) * W;
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
            break;
        }
        }

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
                    double neff = (sw[i] * sw[i]) / std::max(sw2[i], std::numeric_limits<double>::min());
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

        if (include_out_of_range_bins) {
            double first_width = edges.size() > 1 ? (edges[1] - edges[0]) : (domain_max - domain_min);
            double last_width =
                edges.size() > 1 ? (edges[edges.size() - 1] - edges[edges.size() - 2]) : (domain_max - domain_min);
            double underflow_width = 0.5 * first_width;
            double overflow_width = 0.5 * last_width;
            edges.insert(edges.begin(), domain_min - underflow_width);
            edges.push_back(domain_max + overflow_width);
            log::info("DynamicBinning::finalize_edges", "Added underflow/overflow bins spanning",
                      domain_min - underflow_width, "to", domain_max + overflow_width);
        }

        auto last = std::unique(edges.begin(), edges.end());
        edges.erase(last, edges.end());

        if (edges.size() < 2) {
            edges = {domain_min, domain_max};
        }

        for (size_t i = 1; i < edges.size(); ++i) {
            if (!(edges[i] > edges[i - 1])) {
                edges[i] = std::nextafter(edges[i - 1], std::numeric_limits<double>::infinity());
            }
        }

        return BinningDefinition(edges, original_bdef.getVariable(), original_bdef.getTexLabel(), {},
                                 original_bdef.getStratifierKey().str());
    }
};

} // namespace analysis

#endif
