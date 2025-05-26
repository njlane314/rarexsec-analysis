#ifndef RUN_HISTOGRAM_GENERATOR_H
#define RUN_HISTOGRAM_GENERATOR_H

//#define ENABLE_DEBUGGING

#ifdef ENABLE_DEBUGGING
#include <iostream>
#endif

#include <string>
#include <vector>
#include <map>
#include <memory>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TH1D.h"
#include "TString.h"

#include "Binning.h"
#include "HistogramGenerator.h"
#include "Histogram.h"
#include "EventCategory.h"
#include "SampleTypes.h"

namespace AnalysisFramework {

class RunHistGenerator {
public:
    RunHistGenerator(
    const std::map<std::string, std::pair<SampleType, std::vector<ROOT::RDF::RNode>>>& dataframe_dict,
    double data_pot,
    const AnalysisFramework::Binning& binning_def
    ) : data_pot_(data_pot), binning_def_(binning_def) {
#ifdef ENABLE_DEBUGGING
        std::cout << "DEBUG: RunHistGenerator::Constructor - Initializing..." << std::endl;
#endif
        for (const auto& [sample_name, type_rnode_pair] : dataframe_dict) {
            const SampleType& sample_type = type_rnode_pair.first;
            const std::vector<ROOT::RDF::RNode>& rnode_vector = type_rnode_pair.second;

            if (is_sample_mc(sample_type)) {
                for (const auto& rnode : rnode_vector) {
                    ROOT::RDF::RNode non_const_rnode = rnode;
                    std::shared_ptr<ROOT::RDF::RNode> filtered_node;
                    if (!binning_def_.selection_query.IsNull() && !binning_def_.selection_query.IsWhitespace()) {
#ifdef ENABLE_DEBUGGING
                        std::cout << "DEBUG: RunHistGenerator::Constructor - Applying selection query: " << binning_def_.selection_query.Data() << std::endl;
#endif
                        auto filtered_rnode_tmp = non_const_rnode.Filter(binning_def_.selection_query.Data());
                        filtered_node = std::make_shared<ROOT::RDF::RNode>(filtered_rnode_tmp);
                    } else {
                        filtered_node = std::make_shared<ROOT::RDF::RNode>(non_const_rnode);
                    }
                    mc_filtered_dfs.push_back(filtered_node);
                }
            }
        }
#ifdef ENABLE_DEBUGGING
        std::cout << "DEBUG: RunHistGenerator::Constructor - Initialization complete. " << mc_filtered_dfs.size() << " MC RNodes processed." << std::endl;
#endif
    }

    std::map<int, AnalysisFramework::Histogram> GetMonteCarloHists(
    const TString& category_column_name = "event_category",
    double scale_to_pot = 0.0
    ) const {
#ifdef ENABLE_DEBUGGING
        std::cout << "DEBUG: GetMonteCarloHists - High-performance mode: Booking all histograms..." << std::endl;
#endif
        std::map<int, AnalysisFramework::Histogram> final_mc_hists_map;
        if (mc_filtered_dfs.empty()) return final_mc_hists_map;

        std::map<int, std::vector<ROOT::RDF::RResultPtr<TH1D>>> category_hist_futures;
        const std::vector<int> all_categories = AnalysisFramework::GetCategories(category_column_name.Data());

        for (const auto& mc_df : mc_filtered_dfs) {
            if (!mc_df) continue;
            for (int event_category : all_categories) {
                if (event_category == 0) continue;
                TString category_filter = TString::Format("%s == %d", category_column_name.Data(), event_category);
                auto category_df = mc_df->Filter(category_filter.Data());
                auto hist_future = mc_hist_generator_.BookHistogram(category_df, binning_def_, "event_weight");
                category_hist_futures[event_category].push_back(std::move(hist_future));
            }
        }

#ifdef ENABLE_DEBUGGING
        std::cout << "DEBUG: GetMonteCarloHists - All histograms booked. Triggering event loop now..." << std::endl;
#endif
        for (auto& category_future_pair : category_hist_futures) {
            int event_category = category_future_pair.first;
            auto& hist_futures = category_future_pair.second;
            AnalysisFramework::Histogram combined_hist;
            bool is_first_hist_for_category = true;

            for (auto& hist_future : hist_futures) {
                TH1D& root_hist = *hist_future;
                if (root_hist.GetEntries() == 0) continue;

                std::vector<double> counts;
                TMatrixDSym cov_matrix(root_hist.GetNbinsX());
                cov_matrix.Zero();
                for (int i = 1; i <= root_hist.GetNbinsX(); ++i) {
                    counts.push_back(root_hist.GetBinContent(i));
                    double bin_error = root_hist.GetBinError(i);
                    cov_matrix(i-1, i-1) = bin_error * bin_error;
                }
                TString hist_name = TString::Format("mc_hist_cat_%d", event_category);
                std::string label_str = AnalysisFramework::GetLabel(category_column_name.Data(), event_category);
                
                AnalysisFramework::Histogram hist(
                    binning_def_, counts, cov_matrix, hist_name, label_str.c_str(), // Corrected to c_str()
                    AnalysisFramework::GetColorCode(category_column_name.Data(), event_category),
                    AnalysisFramework::GetFillStyle(category_column_name.Data(), event_category),
                    label_str.c_str()
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

#ifdef ENABLE_DEBUGGING
        std::cout << "DEBUG: GetMonteCarloHists - Finished. Returning " << final_mc_hists_map.size() << " histograms." << std::endl;
#endif
        return final_mc_hists_map;
    }

private:
    double data_pot_;
    AnalysisFramework::Binning binning_def_;
    std::vector<std::shared_ptr<ROOT::RDF::RNode>> mc_filtered_dfs;
    AnalysisFramework::HistogramGenerator mc_hist_generator_;
};

}
#endif