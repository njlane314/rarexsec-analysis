#ifndef RUN_HISTOGRAM_GENERATOR_H
#define RUN_HISTOGRAM_GENERATOR_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <algorithm>
#include <set>
#include <numeric> 

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TString.h"
#include "TMatrixDSym.h"
#include "TColor.h"

#include "Binning.h"
#include "HistogramGenerator.h"
#include "Histogram.h"
#include "EventCategories.h" 
#include "SampleTypes.h"

namespace AnalysisFramework {

class RunHistGenerator {
public:
    RunHistGenerator(
        const std::map<std::string, std::pair<AnalysisFramework::SampleType, ROOT::RDF::RNode>>& dataframe_dict,
        double data_pot,
        const AnalysisFramework::Binning& binning_def
    ) : data_pot_(data_pot), binning_def_(binning_def) {
        for (const auto& [sample_name, type_df_pair] : dataframe_dict) {
            const AnalysisFramework::SampleType& sample_type = type_df_pair.first;
            const ROOT::RDF::RNode& rnode = type_df_pair.second;

            if (is_sample_mc(sample_type)) {
                std::shared_ptr<ROOT::RDF::RNode> filtered_node;
                if (!binning_def_.selection_query.IsNull() && !binning_def_.selection_query.IsWhitespace()) {
                    filtered_node = std::make_shared<ROOT::RDF::RNode>(
                        rnode.Filter(binning_def_.selection_query.Data())
                    );
                } else {
                    filtered_node = std::make_shared<ROOT::RDF::RNode>(rnode);
                }
                mc_filtered_dfs.push_back(filtered_node);
            }
        }
    }

    std::map<AnalysisFramework::EventCategory, AnalysisFramework::Histogram> GetMonteCarloHists(
    const TString& category_column_name = "event_category",
    double scale_to_pot = 0.0
    ) const {
        std::map<AnalysisFramework::EventCategory, AnalysisFramework::Histogram> final_mc_hists_map;

        if (mc_filtered_dfs.empty()) 
            return final_mc_hists_map;

        for (const auto& mc_df : mc_filtered_dfs) {
            if (!mc_df) continue;

            auto unique_categories_ptr = mc_df->Distinct<int>(category_column_name.Data());
            std::vector<int> unique_categories = unique_categories_ptr.GetValue();

            for (int event_category : unique_categories) {
                TString category_filter = TString::Format("%s == %d", category_column_name.Data(), event_category);
                ROOT::RDF::RNode category_df = mc_df->Filter(category_filter.Data());

                std::string label = AnalysisFramework::GetLabel(category_column_name.Data(), event_category);
                int color_code = AnalysisFramework::GetColorCode(category_column_name.Data(), event_category);
                int fill_style = AnalysisFramework::GetFillStyle(category_column_name.Data(), event_category);

                TString hist_name = TString::Format("mc_hist_cat_%d", event_category);
                TString hist_title = label.c_str();
                TString plot_color = TString::Format("%d", color_code);
                int plot_hatch = fill_style;
                TString tex_str = label.c_str();

                AnalysisFramework::Histogram hist = mc_hist_generator_.GenerateHistogram(
                    category_df,
                    binning_def_,
                    "event_weight",
                    hist_name,
                    hist_title,
                    plot_color,
                    plot_hatch,
                    tex_str
                );

                if (final_mc_hists_map.find(event_category) == final_mc_hists_map.end()) {
                    final_mc_hists_map[event_category] = hist;
                } else {
                    final_mc_hists_map[event_category] += hist; 
                }
            }
        }

        if (scale_to_pot > 0.0 && data_pot_ > 0.0) {
            double scale_factor = scale_to_pot / data_pot_;
            for (auto& pair : final_mc_hists_map) {
                pair.second = pair.second * scale_factor; 
            }
        }

        return final_mc_hists_map;
    }

private:
    double data_pot_;
    AnalysisFramework::Binning binning_def_;
    std::vector<std::shared_ptr<ROOT::RDF::RNode>> mc_filtered_dfs;
    AnalysisFramework::HistogramGenerator mc_hist_generator_;
};

} 

#endif // RUN_HISTOGRAM_GENERATOR_H
