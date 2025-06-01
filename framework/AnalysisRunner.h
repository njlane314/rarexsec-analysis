#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <iostream>
#include <future>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TH1.h"

#include "DataManager.h"
#include "AnalysisSpace.h"
#include "Binning.h"
#include "Histogram.h"
#include "SystematicsController.h"
#include "EventCategories.h"

namespace AnalysisFramework {

struct AnalysisResult {
    Histogram total_mc_hist;
    Histogram data_hist;
    double data_pot = 0.0;
    bool blinded = true; 
    std::map<std::string, Histogram> mc_breakdown;
    std::map<std::string, TMatrixDSym> systematic_covariance_breakdown;
    std::map<std::string, std::map<std::string, Histogram>> systematic_variations;
};

class AnalysisRunner {
public:
    struct Parameters {
        DataManager& data_manager;
        AnalysisSpace& analysis_space;
        SystematicsController& systematics_controller;
        double pot_scale_factor = 1.0;
        std::string category_column = "analysis_channel";
    };

    explicit AnalysisRunner(Parameters params) : params_(params) {}

    std::map<std::string, AnalysisResult> run() {
        this->generateTasks();
        this->bookHistograms();

        auto final_results = this->processResults();
        return final_results;
    }

private:
    Parameters params_;
    std::vector<Binning> plot_tasks_;
    std::map<std::string, Binning> plot_task_map_;
    
    std::map<std::string, ROOT::RDF::RResultPtr<TH1D>> data_futures_;
    std::map<std::string, std::map<std::string, std::map<int, ROOT::RDF::RResultPtr<TH1D>>>> mc_category_futures_;

    void generateTasks() {
        const auto& variables = params_.analysis_space.getVariables();
        const auto& regions = params_.analysis_space.getRegions();

        for (const auto& [var_name, var_props] : variables) {
            for (const auto& [reg_name, reg_props] : regions) {
                Binning::Parameters p;
                p.variable = var_props.branch_expression.c_str();
                p.variable_tex = var_props.axis_label.c_str();
                p.variable_tex_short = var_props.axis_label_short.c_str();
                p.selection_key = reg_props.selection_key;
                p.preselection_key = reg_props.preselection_key;
                p.selection_tex = reg_props.title.c_str();
                p.selection_tex_short = reg_props.title_short.c_str();

                std::visit([&p](auto&& arg) {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, UniformBinning>) {
                        p.number_of_bins = arg.n_bins;
                        p.range = {arg.low, arg.high};
                        p.is_log = arg.is_log;
                    } else if constexpr (std::is_same_v<T, VariableBinning>) {
                        p.bin_edges = arg.edges;
                        p.is_log = arg.is_log;
                    }
                }, var_props.binning);

                std::string task_id = var_name + "@" + reg_name;
                plot_tasks_.emplace_back(p);
                plot_task_map_[task_id] = plot_tasks_.back();
            }
        }
    }

    void bookHistograms() {
        const auto all_categories = GetCategories(params_.category_column);
        const auto det_var_nodes = params_.data_manager.getAssociatedVariations();

        for (const auto& [sample_key, sample_info] : params_.data_manager.getAllSamples()) {
            ROOT::RDF::RNode df = sample_info.getDataFrame();
            
            for (const auto& [task_id, task_binning] : plot_task_map_) {
                auto main_filtered_df = df.Filter(task_binning.selection_query.Data());
                TH1D model(task_id.c_str(), task_id.c_str(), task_binning.nBins(), task_binning.bin_edges.data());

                if (sample_info.isMonteCarlo()) {
                     for (const int category_id : all_categories) {
                        if (category_id == 0) continue;
                        TString category_filter = TString::Format("%s == %d", params_.category_column.c_str(), category_id);
                        auto category_df = main_filtered_df.Filter(category_filter.Data());
                        mc_category_futures_[task_id][sample_key][category_id] = category_df.Histo1D(model, task_binning.variable.Data(), "event_weight_cv");
                    }
                    params_.systematics_controller.bookVariations(
                        task_id, sample_key, df, det_var_nodes, task_binning,
                        task_binning.selection_query.Data(), params_.category_column);

                } else { 
                    if (!params_.data_manager.isBlinded()){
                        data_futures_[task_id] = main_filtered_df.Histo1D(model, task_binning.variable.Data(), "event_weight_cv");
                    }
                }
            }
        }
    }

    std::map<std::string, AnalysisResult> processResults() {
        std::map<std::string, AnalysisResult> final_results;
        const auto all_categories = GetCategories(params_.category_column);

        for (const auto& [task_id, task_binning] : plot_task_map_) {
            AnalysisResult result;
            result.blinded = params_.data_manager.isBlinded();
            result.data_pot = params_.data_manager.getDataPOT();
            
            if (!result.blinded && data_futures_.count(task_id)) {
                result.data_hist = Histogram(task_binning, *data_futures_.at(task_id).GetPtr(), "data_hist", "Data");
            }

            Histogram total_mc_nominal(task_binning, "total_mc", "Total MC");
            result.mc_breakdown.clear();

            for (const int category_id : all_categories) {
                if (category_id == 0) continue;
                std::string category_label = GetLabel(params_.category_column, category_id);
                Histogram category_hist(task_binning, category_label, category_label);

                for (const auto& [sample_key, sample_info] : params_.data_manager.getAllSamples()) {
                    if (sample_info.isMonteCarlo()) {
                         if (mc_category_futures_.count(task_id) && 
                            mc_category_futures_.at(task_id).count(sample_key) &&
                            mc_category_futures_.at(task_id).at(sample_key).count(category_id)) {
                            auto hist_ptr = mc_category_futures_.at(task_id).at(sample_key).at(category_id).GetPtr();
                            if (hist_ptr) {
                                Histogram temp_hist(task_binning, *hist_ptr);
                                category_hist = category_hist + temp_hist;
                            }
                        }
                    }
                }

                if (category_hist.sum() > 1e-6) {
                    result.mc_breakdown[category_label] = category_hist;
                    total_mc_nominal = total_mc_nominal + category_hist;
                }
            }
            result.total_mc_hist = total_mc_nominal;

            std::map<std::string, TMatrixDSym> total_syst_breakdown;
            for (const int category_id : all_categories) {
                if (category_id == 0) continue;
                auto per_category_cov_map = params_.systematics_controller.computeAllCovariances(category_id, result.total_mc_hist, task_binning, params_.category_column);

                for (const auto& [syst_name, cov_matrix] : per_category_cov_map) {
                    if (total_syst_breakdown.find(syst_name) == total_syst_breakdown.end()) {
                        total_syst_breakdown[syst_name].ResizeTo(result.total_mc_hist.nBins(), result.total_mc_hist.nBins());
                        total_syst_breakdown[syst_name].Zero();
                    }
                    total_syst_breakdown[syst_name] += cov_matrix;
                }
            }
            
            for(const auto& [syst_name, total_cov] : total_syst_breakdown){
                result.total_mc_hist.addCovariance(total_cov);
                result.systematic_covariance_breakdown.emplace(syst_name, total_cov);
            }

            for (const int category_id : all_categories) {
                if (category_id == 0) continue;
                auto per_category_varied_hists = params_.systematics_controller.getAllVariedHistograms(category_id, task_binning, params_.category_column);
                for(const auto& [syst_name, varied_hists] : per_category_varied_hists){
                    for(const auto& [var_name, hist] : varied_hists){
                         if (result.systematic_variations[syst_name].find(var_name) == result.systematic_variations[syst_name].end()) {
                                result.systematic_variations[syst_name][var_name] = hist;
                            } else {
                                result.systematic_variations[syst_name][var_name] = result.systematic_variations[syst_name][var_name] + hist;
                            }
                    }
                }
            }

            if (params_.pot_scale_factor != 1.0) {
                 result.total_mc_hist = result.total_mc_hist * params_.pot_scale_factor;
                 for(auto& pair : result.mc_breakdown){
                     pair.second = pair.second * params_.pot_scale_factor;
                 }
                 for(auto& syst_pair : result.systematic_variations){
                    for(auto& var_pair : syst_pair.second){
                        var_pair.second = var_pair.second * params_.pot_scale_factor;
                    }
                 }
            }

            final_results[task_id] = result;
        }
        return final_results;
    }
};

}

#endif