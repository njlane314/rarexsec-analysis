#ifndef ADAPTIVE_BINNING_CALCULATOR_H
#define ADAPTIVE_BINNING_CALCULATOR_H

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RVec.hxx"

#include "AnalysisLogger.h"
#include "BinningDefinition.h"

namespace analysis {

class DynamicBinning {
public:
    static BinningDefinition calculate(std::vector<ROOT::RDF::RNode> nodes,
                                       const BinningDefinition& original_bdef,
                                       const std::string& weight_col = "nominal_event_weight",
                                       double min_neff_per_bin = 400.0,
                                       double quantile_low = 0.0,
                                       double quantile_high = 1.0,
                                       bool include_out_of_range_bins = false)
    {
        if (nodes.empty()) {
            log::warn("DynamicBinning::calculate", "Cannot calculate bins: RNode vector is empty.");
            return original_bdef;
        }

        const std::string& branch = original_bdef.getVariable();
        std::string typeName = nodes.front().GetColumnType(branch);

        if (typeName == "double" || typeName == "Float64_t" || typeName == "Double_t") {
            return calculate_scalar<double>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName == "float" || typeName == "Float32_t" || typeName == "Float_t") {
            return calculate_scalar<float>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName == "int" || typeName == "Int_t") {
            return calculate_scalar<int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName == "unsigned int" || typeName == "UInt_t") {
            return calculate_scalar<unsigned int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName == "long" || typeName == "Long64_t" || typeName == "long long") {
            return calculate_scalar<long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName.find("ROOT::VecOps::RVec<double>") != std::string::npos
                   || typeName.find("ROOT::RVec<double>") != std::string::npos
                   || typeName.find("vector<double>") != std::string::npos) {
            return calculate_vector<double>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName.find("ROOT::VecOps::RVec<float>") != std::string::npos
                   || typeName.find("ROOT::RVec<float>") != std::string::npos
                   || typeName.find("vector<float>") != std::string::npos) {
            return calculate_vector<float>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName.find("ROOT::VecOps::RVec<int>") != std::string::npos
                   || typeName.find("ROOT::RVec<int>") != std::string::npos
                   || typeName.find("vector<int>") != std::string::npos) {
            return calculate_vector<int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName.find("ROOT::VecOps::RVec<unsigned int>") != std::string::npos
                   || typeName.find("ROOT::RVec<unsigned int>") != std::string::npos
                   || typeName.find("vector<unsigned int>") != std::string::npos) {
            return calculate_vector<unsigned int>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else if (typeName.find("ROOT::VecOps::RVec<long long>") != std::string::npos
                   || typeName.find("ROOT::RVec<long long>") != std::string::npos
                   || typeName.find("vector<long long>") != std::string::npos
                   || typeName.find("vector<Long64_t>") != std::string::npos) {
            return calculate_vector<long long>(std::move(nodes), original_bdef, weight_col, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
        } else {
            log::fatal("DynamicBinning::calculate", "Unsupported type for dynamic binning:", typeName);
            return original_bdef;
        }
    }

private:
    template<typename T>
    static BinningDefinition calculate_scalar(std::vector<ROOT::RDF::RNode> nodes,
                                              const BinningDefinition& original_bdef,
                                              const std::string& weight_col,
                                              double min_neff_per_bin,
                                              double quantile_low,
                                              double quantile_high,
                                              bool include_out_of_range_bins)
    {
        std::vector<std::pair<double,double>> xw;
        xw.reserve(262144);
        double sumw = 0.0;
        double sumw2 = 0.0;
        const std::string& branch = original_bdef.getVariable();

        for (auto& n : nodes) {
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
                        sumw2 += w*w;
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
        return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
    }

    template<typename T>
    static BinningDefinition calculate_vector(std::vector<ROOT::RDF::RNode> nodes,
                                              const BinningDefinition& original_bdef,
                                              const std::string& weight_col,
                                              double min_neff_per_bin,
                                              double quantile_low,
                                              double quantile_high,
                                              bool include_out_of_range_bins)
    {
        std::vector<std::pair<double,double>> xw;
        xw.reserve(262144);
        double sumw = 0.0;
        double sumw2 = 0.0;
        const std::string& branch = original_bdef.getVariable();

        for (auto& n : nodes) {
            bool has_w = n.HasColumn(weight_col);
            auto vecs = n.template Take<ROOT::RVec<T>>(branch);
            if (has_w) {
                auto ws = n.template Take<double>(weight_col);
                for (size_t i = 0; i < vecs->size(); ++i) {
                    double w = (*ws)[i];
                    if (!std::isfinite(w) || w <= 0.0) continue;
                    for (auto val : (*vecs)[i]) {
                        double x = static_cast<double>(val);
                        if (std::isfinite(x)) {
                            xw.emplace_back(x, w);
                            sumw += w;
                            sumw2 += w*w;
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
        return finalize_edges(xw, sumw, sumw2, original_bdef, min_neff_per_bin, quantile_low, quantile_high, include_out_of_range_bins);
    }

    static BinningDefinition finalize_edges(std::vector<std::pair<double,double>>& xw,
                                            double sumw,
                                            double sumw2,
                                            const BinningDefinition& original_bdef,
                                            double min_neff_per_bin,
                                            double quantile_low,
                                            double quantile_high,
                                            bool include_out_of_range_bins)
    {
        const auto& domain_edges = original_bdef.getEdges();
        double domain_min = domain_edges.front();
        double domain_max = domain_edges.back();

        size_t before_filter = xw.size();
        xw.erase(std::remove_if(xw.begin(), xw.end(), [&](const auto& p){
            double x = p.first;
            double w = p.second;
            return !std::isfinite(x) || !std::isfinite(w) || w <= 0.0 || x < domain_min || x > domain_max;
        }), xw.end());
        size_t removed_invalid = before_filter - xw.size();
        if (removed_invalid > 0) {
            log::info("DynamicBinning::finalize_edges", "Discarded", removed_invalid, "entries outside domain or non-finite");
        }

        if (xw.size() < 2) {
            return BinningDefinition({domain_min, domain_max}, original_bdef.getVariable(), original_bdef.getTexLabel(), {}, original_bdef.getStratifierKey().str());
        }

        std::sort(xw.begin(), xw.end(), [](const auto& a, const auto& b){ return a.first < b.first; });

        // recompute sums after filtering
        sumw = 0.0;
        sumw2 = 0.0;
        for (const auto& p : xw) {
            sumw += p.second;
            sumw2 += p.second * p.second;
        }

        if (quantile_low > 0.0 || quantile_high < 1.0) {
            double lower_thresh = quantile_low * sumw;
            double upper_thresh = quantile_high * sumw;
            double cum = 0.0;
            size_t start = 0;
            while (start < xw.size() && cum + xw[start].second <= lower_thresh) {
                cum += xw[start].second;
                ++start;
            }
            cum = sumw;
            size_t end = xw.size();
            while (end > start && cum - xw[end-1].second >= upper_thresh) {
                cum -= xw[end-1].second;
                --end;
            }
            size_t removed_trim = start + (xw.size() - end);
            if (removed_trim > 0) {
                log::info("DynamicBinning::finalize_edges", "Trimmed", removed_trim, "entries outside quantiles", quantile_low, quantile_high);
            }
            xw.erase(xw.begin() + end, xw.end());
            xw.erase(xw.begin(), xw.begin() + start);

            sumw = 0.0;
            sumw2 = 0.0;
            for (const auto& p : xw) {
                sumw += p.second;
                sumw2 += p.second * p.second;
            }
        }

        if (xw.size() < 2 || sumw <= 0.0) {
            return BinningDefinition({domain_min, domain_max}, original_bdef.getVariable(), original_bdef.getTexLabel(), {}, original_bdef.getStratifierKey().str());
        }

        double xmin = xw.front().first;
        double xmax = xw.back().first;
        log::info("DynamicBinning::finalize_edges", "Resolved data range for", original_bdef.getVariable(), ":", xmin, "to", xmax);

        double neff_total = (sumw*sumw) / std::max(sumw2, std::numeric_limits<double>::min());
        int target_bins = static_cast<int>(std::floor(neff_total / std::max(min_neff_per_bin, 1.0)));
        if (target_bins < 1) target_bins = 1;

        std::vector<double> edges;
        edges.reserve(static_cast<size_t>(target_bins) + 3);
        edges.push_back(xw.front().first);

        if (target_bins > 1) {
            double W = sumw;
            double cum = 0.0;
            size_t idx = 0;
            for (int k = 1; k < target_bins; ++k) {
                double thresh = (static_cast<double>(k) / static_cast<double>(target_bins)) * W;
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

        // enforce the user-specified domain for the main histogram axis
        edges.front() = domain_min;
        edges.back()  = domain_max;

        // Optionally append explicit underflow/overflow bins with a width
        // matching the first and last in-domain bins so they are visible on plots
        if (include_out_of_range_bins) {
            double first_width = edges.size() > 1 ? (edges[1] - edges[0]) : (domain_max - domain_min);
            double last_width  = edges.size() > 1 ? (edges[edges.size()-1] - edges[edges.size()-2]) : (domain_max - domain_min);
            double underflow_width = 0.5 * first_width;
            double overflow_width  = 0.5 * last_width;
            edges.insert(edges.begin(), domain_min - underflow_width);
            edges.push_back(domain_max + overflow_width);
            log::info("DynamicBinning::finalize_edges", "Added underflow/overflow bins spanning", domain_min - underflow_width, "to", domain_max + overflow_width);
        }

        auto last = std::unique(edges.begin(), edges.end());
        edges.erase(last, edges.end());

        if (edges.size() < 2) {
            edges = {domain_min, domain_max};
        }

        for (size_t i = 1; i < edges.size(); ++i) {
            if (!(edges[i] > edges[i-1])) {
                 edges[i] = std::nextafter(edges[i-1], std::numeric_limits<double>::infinity());
            }
        }

        return BinningDefinition(edges, original_bdef.getVariable(), original_bdef.getTexLabel(), {}, original_bdef.getStratifierKey().str());
    }
};

}

#endif
