#ifndef RUN_HISTOGRAM_GENERATOR_H
#define RUN_HISTOGRAM_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <numeric>
#include <iostream>
#include <chrono>
#include <ctime>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "ROOT/TTreeProcessorMT.hxx"
#include "TH1D.h"
#include "TString.h"
#include "TMatrixDSym.h"

#include "Binning.h"
#include "HistogramGenerator.h"
#include "Histogram.h"
#include "EventCategory.h"
#include "SampleTypes.h"

namespace AnalysisFramework {

void Log(const std::string& message) {
    auto now = std::chrono::system_clock::now();
    std::time_t time = std::chrono::system_clock::to_time_t(now);
    char time_str[26];
    ctime_r(&time, time_str);
    time_str[24] = '\0';
    std::cout << "[" << time_str << "] " << message << std::endl;
}

using SystematicBreakdown = std::map<int, std::map<std::string, TMatrixDSym>>;
using NominalHistograms = std::map<int, AnalysisFramework::Histogram>;

class RunHistGenerator {
public:
    struct Parameters {
        std::map<std::string, std::pair<SampleType, std::vector<ROOT::RDF::RNode>>> dataframes;
        double data_pot;
        AnalysisFramework::Binning& binning;
    };

    RunHistGenerator(const Parameters& p)
    : data_pot_(p.data_pot), binning_(p.binning) {
        Log("RunHistGenerator: Initializing and pre-filtering dataframes...");
        for (const auto& [sample_name, type_rnode_pair] : p.dataframes) {
            const SampleType& sample_type = type_rnode_pair.first;
            const std::vector<ROOT::RDF::RNode>& rnode_vector = type_rnode_pair.second;

            if (is_sample_mc(sample_type)) {
                for (const auto& rnode : rnode_vector) {
                    ROOT::RDF::RNode non_const_rnode = rnode;
                    std::shared_ptr<ROOT::RDF::RNode> filtered_node;
                    if (!p.binning.selection_query.IsNull() && !p.binning.selection_query.IsWhitespace()) {
                        auto filtered_rnode_tmp = non_const_rnode.Filter(p.binning.selection_query.Data());
                        filtered_node = std::make_shared<ROOT::RDF::RNode>(filtered_rnode_tmp);
                    } else {
                        filtered_node = std::make_shared<ROOT::RDF::RNode>(non_const_rnode);
                    }
                    mc_filtered_dfs_.push_back(filtered_node);
                }
            }
        }
        Log("RunHistGenerator: Initialization complete. Found " + std::to_string(mc_filtered_dfs_.size()) + " MC dataframes.");
    }

    std::map<int, AnalysisFramework::Histogram> GetMonteCarloHists(
        const TString& category_column_name = "event_category",
        double scale_to_pot = 0.0
    ) const {
        Log("Starting nominal-only histogram generation...");
        std::map<int, AnalysisFramework::Histogram> final_mc_hists_map;
        if (mc_filtered_dfs_.empty()) return final_mc_hists_map;

        std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> category_hist_futures;
        const std::vector<int> all_categories = AnalysisFramework::GetCategories(category_column_name.Data());

        Log("Booking nominal histograms for " + std::to_string(all_categories.size()) + " categories...");
        for (const auto& mc_df_ptr : mc_filtered_dfs_) {
            if (!mc_df_ptr) continue;
            ROOT::RDF::RNode mc_df = *mc_df_ptr;
            for (int event_category : all_categories) {
                if (event_category == 0) continue;
                TString category_filter_str = TString::Format("%s == %d", category_column_name.Data(), event_category);
                auto category_df = mc_df.Filter(category_filter_str.Data());
                auto hist_future = mc_hist_generator_.BookHistogram(category_df, binning_, "event_weight");
                category_hist_futures[event_category].push_back(std::move(hist_future));
            }
        }
        Log("Booking complete. Triggering event loop and processing results...");

        for (auto& category_future_pair : category_hist_futures) {
            int event_category = category_future_pair.first;
            Log("Processing results for category: " + std::to_string(event_category));
            auto& hist_futures = category_future_pair.second;
            AnalysisFramework::Histogram combined_hist;
            bool is_first_hist_for_category = true;

            for (auto& hist_future : hist_futures) {
                TH1D& root_hist = *hist_future;
                if (root_hist.GetEntries() == 0 && root_hist.Integral() == 0) continue;

                std::vector<double> counts;
                TMatrixDSym cov_matrix(root_hist.GetNbinsX());
                cov_matrix.Zero();
                for (int i = 1; i <= root_hist.GetNbinsX(); ++i) {
                    counts.push_back(root_hist.GetBinContent(i));
                    double bin_error = root_hist.GetBinError(i);
                    cov_matrix(i-1, i-1) = bin_error * bin_error;
                }
                TString hist_name = TString::Format("mc_hist_cat_%d", event_category);
                TString label_str = GetLabel(category_column_name.Data(), event_category);

                AnalysisFramework::Histogram hist(
                    binning_,
                    counts,
                    cov_matrix,
                    hist_name,
                    label_str,
                    GetColorCode(category_column_name.Data(), event_category),
                    GetFillStyle(category_column_name.Data(), event_category),
                    label_str
                );

                if (is_first_hist_for_category) {
                    combined_hist = hist;
                    is_first_hist_for_category = false;
                } else {
                    combined_hist = combined_hist + hist;
                }
            }
            if (!is_first_hist_for_category) {
                final_mc_hists_map[event_category] = combined_hist;
            }
        }

        if (scale_to_pot > 0.0 && data_pot_ > 0.0) {
            double scale_factor = scale_to_pot / data_pot_;
            for (auto& pair : final_mc_hists_map) {
                pair.second = pair.second * scale_factor;
            }
        }
        Log("Nominal-only histogram generation finished.");
        return final_mc_hists_map;
    }


    std::map<int, AnalysisFramework::Histogram> GetMonteCarloHistsWithSystematics(
        const std::vector<std::string>& multisim_sources,
        const TString& category_column_name = "event_category",
        double scale_to_pot = 0.0
    ) const {
        Log("Starting COMBINED histogram generation with systematics...");
        if (mc_filtered_dfs_.empty()) {
            Log("No MC dataframes to process. Returning empty map.");
            return {};
        }

        const auto all_categories = AnalysisFramework::GetCategories(category_column_name.Data());
        std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> nominal_futures;
        std::map<std::string, std::map<int, std::vector<std::vector<ROOT::RDF::RResultPtr<TH1D>>>>> syst_futures;
        std::map<std::string, int> n_universes_map;

        Log("--- Starting Booking Phase ---");
        ROOT::EnableImplicitMT();
        ROOT::DisableImplicitMT();
        Log("Temporarily disabled multithreading to determine universe counts...");
        for (const auto& source : multisim_sources) {
            for (const auto& df_ptr : mc_filtered_dfs_) {
                if (df_ptr && df_ptr->HasColumn(source)) {
                    auto n_univ_proxy = df_ptr->Range(1).Take<ROOT::RVec<unsigned short>>(source);
                    if (!n_univ_proxy->empty() && !n_univ_proxy->at(0).empty()) {
                        n_universes_map[source] = n_univ_proxy->at(0).size();
                        Log("Found " + std::to_string(n_universes_map[source]) + " universes for source: " + source);
                        break;
                    }
                }
            }
            if (n_universes_map.find(source) == n_universes_map.end()) {
                 Log("Warning: Could not determine number of universes for source: " + source);
                 n_universes_map[source] = 0;
            }
        }
        ROOT::EnableImplicitMT();
        Log("Re-enabled multithreading. Proceeding with booking...");

        for (const auto& mc_df_ptr : mc_filtered_dfs_) {
            for (int cat_id : all_categories) {
                if (cat_id == 0) continue;
                TString cat_filter = TString::Format("%s == %d", category_column_name.Data(), cat_id);
                auto category_df = mc_df_ptr->Filter(cat_filter.Data());

                nominal_futures[cat_id].push_back(mc_hist_generator_.BookHistogram(category_df, binning_, "event_weight"));

                for (const auto& source : multisim_sources) {
                    if (n_universes_map[source] > 0 && category_df.HasColumn(source)) {
                        int n_univ = n_universes_map[source];
                        if (syst_futures[source].find(cat_id) == syst_futures[source].end()) {
                            syst_futures[source][cat_id].resize(n_univ);
                        }
                        const float rescale = 1000.0f;
                        for (int u = 0; u < n_univ; ++u) {
                            auto weight_expr = TString::Format("event_weight * ((%s.size() > (size_t)%d) ? (static_cast<float>(%s[%d]) / %.1f) : 1.0f)", source.c_str(), u, source.c_str(), u, rescale);
                            auto df_with_weight = category_df.Define("univ_weight_temp", weight_expr.Data());
                            syst_futures[source][cat_id][u].push_back(mc_hist_generator_.BookHistogram(df_with_weight, binning_, "univ_weight_temp"));
                        }
                    }
                }
            }
        }
        Log("--- Booking Phase Complete. Triggering Single Event Loop... ---");


        std::map<int, Histogram> final_hists_map;
        for (int cat_id : all_categories) {
            if (cat_id == 0) continue;

            Log("Processing results for category: " + std::to_string(cat_id));

            auto nominal_th1d = std::make_unique<TH1D>(
                TString::Format("nominal_cat%d", cat_id), "", binning_.nBins(), binning_.bin_edges.data()
            );
            nominal_th1d->SetDirectory(nullptr);
            bool has_nominal_events = false;
            if(nominal_futures.count(cat_id)) {
                for (auto& future : nominal_futures.at(cat_id)) {
                    if(future->GetEntries() > 0) has_nominal_events = true;
                    nominal_th1d->Add(&*future);
                }
            }
            if(!has_nominal_events) {
                Log("Skipping category " + std::to_string(cat_id) + " (no nominal events).");
                continue;
            }

            std::vector<double> counts;
            TMatrixDSym stat_cov_matrix(nominal_th1d->GetNbinsX());
            stat_cov_matrix.Zero();
            for (int i = 1; i <= nominal_th1d->GetNbinsX(); ++i) {
                counts.push_back(nominal_th1d->GetBinContent(i));
                double bin_error = nominal_th1d->GetBinError(i);
                stat_cov_matrix(i-1, i-1) = bin_error * bin_error;
            }
            TString label_str = GetLabel(category_column_name.Data(), cat_id);
            Histogram final_hist(
                binning_,
                counts,
                stat_cov_matrix,
                TString::Format("hist_cat_%d", cat_id),
                label_str,
                GetColorCode(category_column_name.Data(), cat_id),
                GetFillStyle(category_column_name.Data(), cat_id),
                label_str
            );

            for (const auto& source : multisim_sources) {
                if (!syst_futures.count(source) || !syst_futures.at(source).count(cat_id)) continue;

                std::vector<std::unique_ptr<TH1D>> universe_hists;
                const int n_univ = n_universes_map[source];
                universe_hists.reserve(n_univ);

                for (int u = 0; u < n_univ; ++u) {
                    auto univ_hist = std::make_unique<TH1D>(TString::Format("univ_cat%d_src%s_u%d", cat_id, source.c_str(), u),"", binning_.nBins(), binning_.bin_edges.data());
                    univ_hist->SetDirectory(nullptr);
                    for (auto& future : syst_futures.at(source).at(cat_id).at(u)) {
                        univ_hist->Add(&*future);
                    }
                    universe_hists.push_back(std::move(univ_hist));
                }

                Log("Calculating covariance for source '" + source + "' in category " + std::to_string(cat_id));
                TMatrixDSym syst_cov = ComputeCovarianceFromUniverses(universe_hists, nominal_th1d.get());
                final_hist.addCovariance(syst_cov);
            }
            final_hists_map[cat_id] = final_hist;
        }

        if (scale_to_pot > 0.0 && data_pot_ > 0.0) {
            double scale_factor = scale_to_pot / data_pot_;
            Log("Scaling all histograms to POT: " + std::to_string(scale_to_pot));
            for (auto& pair : final_hists_map) {
                pair.second = pair.second * scale_factor;
            }
        }
        Log("--- Combined histogram generation finished. ---");
        return final_hists_map;
    }

    std::pair<NominalHistograms, SystematicBreakdown> GetHistogramsAndSystematicBreakdown(
        const std::vector<std::string>& multisim_sources,
        const TString& category_column_name = "event_category"
    ) const {
        Log("Starting histogram and systematics breakdown generation...");
        if (mc_filtered_dfs_.empty()) {
            Log("No MC dataframes to process. Returning empty maps.");
            return {};
        }

        const auto all_categories = AnalysisFramework::GetCategories(category_column_name.Data());
        std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> nominal_futures;
        std::map<std::string, std::map<int, std::vector<std::vector<ROOT::RDF::RResultPtr<TH1D>>>>> syst_futures;
        std::map<std::string, int> n_universes_map;

        Log("--- Starting Booking Phase ---");
        ROOT::EnableImplicitMT();
        ROOT::DisableImplicitMT();
        Log("Temporarily disabled multithreading to determine universe counts...");
        for (const auto& source : multisim_sources) {
            for (const auto& df_ptr : mc_filtered_dfs_) {
                if (df_ptr && df_ptr->HasColumn(source)) {
                    auto n_univ_proxy = df_ptr->Range(1).Take<ROOT::RVec<unsigned short>>(source);
                    if (!n_univ_proxy->empty() && !n_univ_proxy->at(0).empty()) {
                        n_universes_map[source] = n_univ_proxy->at(0).size();
                        break;
                    }
                }
            }
            if (n_universes_map.find(source) == n_universes_map.end()) {
                n_universes_map[source] = 0;
            }
        }
        ROOT::EnableImplicitMT();
        Log("Re-enabled multithreading. Proceeding with booking...");

        for (const auto& mc_df_ptr : mc_filtered_dfs_) {
            for (int cat_id : all_categories) {
                if (cat_id == 0) continue;
                TString cat_filter = TString::Format("%s == %d", category_column_name.Data(), cat_id);
                auto category_df = mc_df_ptr->Filter(cat_filter.Data());
                nominal_futures[cat_id].push_back(mc_hist_generator_.BookHistogram(category_df, binning_, "event_weight"));
                for (const auto& source : multisim_sources) {
                    if (n_universes_map[source] > 0 && category_df.HasColumn(source)) {
                        int n_univ = n_universes_map[source];
                        if (syst_futures[source].find(cat_id) == syst_futures[source].end()) {
                            syst_futures[source][cat_id].resize(n_univ);
                        }
                        const float rescale = 1000.0f;
                        for (int u = 0; u < n_univ; ++u) {
                            auto weight_expr = TString::Format("event_weight * ((%s.size() > (size_t)%d) ? (static_cast<float>(%s[%d]) / %.1f) : 1.0f)", source.c_str(), u, source.c_str(), u, rescale);
                            auto df_with_weight = category_df.Define("univ_weight_temp", weight_expr.Data());
                            syst_futures[source][cat_id][u].push_back(mc_hist_generator_.BookHistogram(df_with_weight, binning_, "univ_weight_temp"));
                        }
                    }
                }
            }
        }
        Log("--- Booking Phase Complete. Triggering Event Loop... ---");

        NominalHistograms nominal_hists_map;
        SystematicBreakdown syst_breakdown_map;

        for (int cat_id : all_categories) {
            if (cat_id == 0) continue;

            Log("Processing results for category: " + std::to_string(cat_id));
            auto nominal_th1d = std::make_unique<TH1D>(TString::Format("nominal_cat%d", cat_id), "", binning_.nBins(), binning_.bin_edges.data());
            nominal_th1d->SetDirectory(nullptr);
            bool has_nominal_events = false;
            if (nominal_futures.count(cat_id)) {
                for (auto& future : nominal_futures.at(cat_id)) {
                    if(future->GetEntries() > 0) has_nominal_events = true;
                    nominal_th1d->Add(&*future);
                }
            }
            if (!has_nominal_events) continue;

            std::vector<double> counts;
            TMatrixDSym stat_cov_matrix(nominal_th1d->GetNbinsX());
            stat_cov_matrix.Zero();
            for (int i = 1; i <= nominal_th1d->GetNbinsX(); ++i) {
                counts.push_back(nominal_th1d->GetBinContent(i));
                stat_cov_matrix(i-1, i-1) = nominal_th1d->GetBinError(i) * nominal_th1d->GetBinError(i);
            }
            TString label_str = GetLabel(category_column_name.Data(), cat_id);
            Histogram nominal_hist(
                binning_,
                counts,
                stat_cov_matrix,
                TString::Format("nominal_hist_cat_%d", cat_id),
                label_str,
                GetColorCode(category_column_name.Data(), cat_id),
                GetFillStyle(category_column_name.Data(), cat_id),
                label_str
            );
            nominal_hists_map[cat_id] = nominal_hist;

            for (const auto& source : multisim_sources) {
                if (!syst_futures.count(source) || !syst_futures.at(source).count(cat_id)) continue;

                std::vector<std::unique_ptr<TH1D>> universe_hists;
                const int n_univ = n_universes_map.at(source);
                universe_hists.reserve(n_univ);
                for (int u = 0; u < n_univ; ++u) {
                    auto univ_hist = std::make_unique<TH1D>(TString::Format("univ_cat%d_src%s_u%d", cat_id, source.c_str(), u),"", binning_.nBins(), binning_.bin_edges.data());
                    univ_hist->SetDirectory(nullptr);
                    for (auto& future : syst_futures.at(source).at(cat_id).at(u)) {
                        univ_hist->Add(&*future);
                    }
                    universe_hists.push_back(std::move(univ_hist));
                }

                TMatrixDSym syst_cov = ComputeCovarianceFromUniverses(universe_hists, nominal_th1d.get());
                syst_breakdown_map[cat_id][source] = syst_cov;
            }
        }
        Log("--- Histogram and breakdown generation finished. ---");
        return {nominal_hists_map, syst_breakdown_map};
    }


private:
    const double data_pot_;
    const AnalysisFramework::Binning binning_;

    std::vector<std::shared_ptr<ROOT::RDF::RNode>> mc_filtered_dfs_;
    AnalysisFramework::HistogramGenerator mc_hist_generator_;

    TMatrixDSym ComputeCovarianceFromUniverses(
        const std::vector<std::unique_ptr<TH1D>>& universe_hists,
        const TH1D* nominal_hist
    ) const {
        if (universe_hists.empty() || !nominal_hist) {
            return TMatrixDSym(binning_.nBins());
        }

        const int n_bins = nominal_hist->GetNbinsX();
        if (n_bins == 0) return TMatrixDSym(0);
        const int n_universes = universe_hists.size();
        if (n_universes == 0) return TMatrixDSym(n_bins);

        TMatrixDSym cov_matrix(n_bins);
        cov_matrix.Zero();

        for (int i = 0; i < n_bins; ++i) {
            for (int j = i; j < n_bins; ++j) {
                double sum_val = 0.0;
                const double cv_i = nominal_hist->GetBinContent(i + 1);
                const double cv_j = nominal_hist->GetBinContent(j + 1);

                for (int u_idx = 0; u_idx < n_universes; ++u_idx) {
                    if (!universe_hists[u_idx]) continue;
                    const double univ_i = universe_hists[u_idx]->GetBinContent(i + 1);
                    const double univ_j = universe_hists[u_idx]->GetBinContent(j + 1);
                    sum_val += (univ_i - cv_i) * (univ_j - cv_j);
                }
                if (n_universes > 0) {
                    cov_matrix(i, j) = sum_val / static_cast<double>(n_universes);
                } else {
                    cov_matrix(i,j) = 0;
                }
            }
        }

        for (int i = 0; i < n_bins; ++i) {
            for (int j = 0; j < i; ++j) {
                cov_matrix(i, j) = cov_matrix(j, i);
            }
        }

        return cov_matrix;
    }
};

}
#endif