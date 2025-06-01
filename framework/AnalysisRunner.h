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
    std::map<std::string, Histogram> mc_breakdown;
    std::map<std::string, TMatrixDSym> systematic_covariance_breakdown;
};

class AnalysisRunner {
public:
    struct Parameters {
        DataManager& data_manager;
        AnalysisSpace& analysis_space;
        SystematicsController& systematics_controller;
        double pot_scale_factor = 1.0;
    };

    explicit AnalysisRunner(Parameters params) : params_(params) {}

    std::map<std::string, AnalysisResult> run() {
        this->generateTasks();
        this->bookHistograms();

        for (auto& future : future_handles_) {
            future.GetValue();
        }

        auto final_results = this->processResults();
        return final_results;
    }

private:
    Parameters params_;
    std::vector<Binning> plot_tasks_;
    std::map<std::string, Binning> plot_task_map_;
    std::vector<ROOT::RDF::RResultPtr<TH1D>> future_handles_;
    std::map<std::string, std::map<std::string, ROOT::RDF::RResultPtr<TH1D>>> nominal_futures_;

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
        const auto det_var_nodes = params_.data_manager.getAssociatedVariations();

        for (const auto& [sample_key, sample_info] : params_.data_manager.getAllSamples()) {
            ROOT::RDF::RNode df = sample_info.getDataFrame();

            for (const auto& [task_id, task_binning] : plot_task_map_) {
                auto filtered_df = df.Filter(task_binning.selection_query.Data());
                TH1D model(task_id.c_str(), task_id.c_str(), task_binning.nBins(), task_binning.bin_edges.data());
                auto future = filtered_df.Histo1D(model, task_binning.variable.Data(), "event_weight_cv");
                
                nominal_futures_[task_id][sample_key] = future;
                future_handles_.emplace_back(future);

                if (sample_info.isMonteCarlo()) {
                    params_.systematics_controller.bookVariations(
                        task_id,
                        sample_key,
                        df,
                        det_var_nodes,
                        task_binning,
                        task_binning.selection_query.Data()
                    );
                }
            }
        }
    }

    std::map<std::string, AnalysisResult> processResults() {
        std::map<std::string, AnalysisResult> final_results;

        for (const auto& [task_id, task_binning] : plot_task_map_) {
            AnalysisResult result;
            const auto& all_samples = params_.data_manager.getAllSamples();

            if (all_samples.count("data")) {
                result.data_hist = Histogram(plot_task_map_.at(task_id), nominal_futures_.at(task_id).at("data").GetValue(), "data_hist", "Data Histogram");
            }

            Histogram total_mc_nominal(task_binning, "total_mc", "Total MC");
            for (const auto& [sample_key, sample_info] : all_samples) {
                if (sample_key != "data" && sample_info.isMonteCarlo()) {
                    Histogram hist(plot_task_map_.at(task_id), nominal_futures_.at(task_id).at(sample_key).GetValue(),
                                    sample_key, sample_key);
                    result.mc_breakdown[sample_key] = hist;
                    total_mc_nominal = total_mc_nominal + hist;
                }
            }
            result.total_mc_hist = total_mc_nominal;

            std::map<std::string, TMatrixDSym> total_syst_breakdown;
            auto all_categories = GetCategories("event_category");

            for (const int category_id : all_categories) {
                if (category_id == 0) continue; 
                auto per_category_cov_map = params_.systematics_controller.computeAllCovariances(category_id, result.total_mc_hist, task_binning);

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

            if (params_.pot_scale_factor != 1.0) {
                 result.total_mc_hist = result.total_mc_hist * params_.pot_scale_factor;
                 for(auto& pair : result.mc_breakdown){
                     pair.second = pair.second * params_.pot_scale_factor;
                 }
            }

            final_results[task_id] = result;
        }
        return final_results;
    }
};

}

#endif