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
#include "SystematicsController.h"
#include "AnalysisChannels.h"
#include "AnalysisResult.h"

namespace AnalysisFramework {

class AnalysisRunner {
public:
    struct Parameters {
        DataManager& data_manager;
        AnalysisSpace& analysis_space;
        SystematicsController& systematics_controller;
        double pot_scale_factor = 1.0;
        std::string event_category_column;
        std::string particle_category_column;
        std::string particle_category_scheme;
    };

    explicit AnalysisRunner(Parameters params) : params_(params) {}

    AnalysisPhaseSpace run() {
        this->generateTasks();
        this->bookHistograms();

        auto analysis_phase_space = this->processResults();
        return analysis_phase_space;
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
                p.selection_keys = reg_props.selection_keys;
                p.selection_tex = reg_props.title.c_str();
                p.selection_tex_short = reg_props.title_short.c_str();

                p.is_particle_level = var_props.is_particle_level;
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

                std::string task_key = var_name + "@" + reg_name;
                plot_tasks_.emplace_back(p);
                plot_task_map_[task_key] = plot_tasks_.back();
            }
        }
    }

    void bookHistograms() {
        const auto det_var_nodes = params_.data_manager.getAssociatedVariations();

        for (const auto& [task_key, task_binning] : plot_task_map_) {
            TH1D model(task_key.c_str(), task_key.c_str(), task_binning.nBins(), task_binning.bin_edges.data());

            for (const auto& [sample_key, sample_info] : params_.data_manager.getAllSamples()) {
                ROOT::RDF::RNode df = sample_info.getDataFrame();
                auto main_filtered_df = df.Filter(task_binning.selection_query.Data());

                if (task_binning.is_particle_level) {
                    if (sample_info.isMonteCarlo()) {
                        const auto particle_channel_keys = getChannelKeys(params_.particle_category_scheme);
                        const std::string& var_branch = task_binning.variable.Data();
                        const std::string& cat_branch = params_.particle_category_column;

                        for (const int pdg_code : particle_channel_keys) {
                            if (pdg_code == 0) continue;
                            int pdg_code_abs = std::abs(pdg_code);
                           
                            auto selector = [pdg_code_abs](const ROOT::RVec<float>& var_vec, const ROOT::RVec<int>& pdg_vec) {
                                return var_vec[pdg_vec == pdg_code_abs];
                            };
                            
                            std::string new_col_name = var_branch + "_" + std::to_string(pdg_code_abs);
                            
                            auto category_df = main_filtered_df.Define(new_col_name, selector, {var_branch, cat_branch});

                            mc_category_futures_[task_key][sample_key][pdg_code_abs] = 
                                category_df.Histo1D(model, new_col_name, "base_event_weight");
                        }
                        
                        params_.systematics_controller.bookVariations(
                            task_key, sample_key, df, det_var_nodes, task_binning,
                            task_binning.selection_query.Data(), params_.particle_category_column, params_.particle_category_scheme);
                    }

                } else {
                    if (sample_info.isMonteCarlo()) {
                        const auto analysis_channel_keys = getChannelKeys(params_.event_category_column);
                        for (const int channel_key : analysis_channel_keys) {
                            if (channel_key == 0) continue;
                            TString category_filter = TString::Format("%s == %d", params_.event_category_column.c_str(), channel_key);
                            auto category_df = main_filtered_df.Filter(category_filter.Data());
                            mc_category_futures_[task_key][sample_key][channel_key] = category_df.Histo1D(model, task_binning.variable.Data(), "base_event_weight");
                        }
                        
                        params_.systematics_controller.bookVariations(
                            task_key, sample_key, df, det_var_nodes, task_binning,
                            task_binning.selection_query.Data(), params_.event_category_column, params_.event_category_column);

                    } else {
                        if (!params_.data_manager.isBlinded()){
                            data_futures_[task_key] = main_filtered_df.Histo1D(model, task_binning.variable.Data());
                        }
                    }
                }
            }
        }
    }

    AnalysisPhaseSpace processResults() {
        AnalysisPhaseSpace analysis_phase_space;

        for (const auto& [task_key, task_binning] : plot_task_map_) {
            AnalysisResult result;
            
            result.setBlinded(params_.data_manager.isBlinded());
            result.setPOT(params_.data_manager.getDataPOT());
            result.setBeamKey(params_.data_manager.getBeamKey());
            result.setRuns(params_.data_manager.getRunsToLoad());

            if (!result.isBlinded() && data_futures_.count(task_key)) {
                result.setDataHist(Histogram(task_binning, *data_futures_.at(task_key).GetPtr(), "data_hist", "Data"));
            }

            std::string category_scheme;
            if (task_binning.is_particle_level) {
                category_scheme = params_.particle_category_scheme;
            } else {
                category_scheme = params_.event_category_column;
            }
            const auto analysis_channel_keys = getChannelKeys(category_scheme);

            Histogram total_mc_nominal(task_binning, "total_mc", "Total MC");

            for (const int channel_key : analysis_channel_keys) {
                if (!task_binning.is_particle_level && channel_key == 0) continue;

                std::string category_label = getChannelLabel(category_scheme, channel_key);
                int color = getChannelColourCode(category_scheme, channel_key);
                int fill_style = getChannelFillStyle(category_scheme, channel_key);
                Histogram category_hist(task_binning, category_label, category_label, color, fill_style);

                for (const auto& [sample_key, sample_info] : params_.data_manager.getAllSamples()) {
                    if (sample_info.isMonteCarlo()) {
                        if (mc_category_futures_.count(task_key) &&
                            mc_category_futures_.at(task_key).count(sample_key) &&
                            mc_category_futures_.at(task_key).at(sample_key).count(channel_key)) {
                            auto hist_ptr = mc_category_futures_.at(task_key).at(sample_key).at(channel_key).GetPtr();
                            if (hist_ptr) {
                                category_hist = category_hist + Histogram(task_binning, *hist_ptr);
                            }
                        }
                    }
                }

                if (category_hist.sum() > 1e-6) {
                    result.addChannel(category_label, category_hist);
                    total_mc_nominal = total_mc_nominal + category_hist;
                }
            }
            result.setTotalHist(total_mc_nominal);
            
            if (!task_binning.is_particle_level) {
                const auto nominal_hist_for_cov = result.getTotalHist();
                std::map<std::string, TMatrixDSym> total_syst_breakdown;
                for (const int channel_key : analysis_channel_keys) {
                    if (channel_key == 0) continue;
                    auto per_category_cov_map = params_.systematics_controller.computeAllCovariances(channel_key, nominal_hist_for_cov, task_binning, category_scheme);
                    for (const auto& [syst_name, cov_matrix] : per_category_cov_map) {
                        if (total_syst_breakdown.find(syst_name) == total_syst_breakdown.end()) {
                            total_syst_breakdown[syst_name].ResizeTo(nominal_hist_for_cov.nBins(), nominal_hist_for_cov.nBins());
                            total_syst_breakdown[syst_name].Zero();
                        }
                        total_syst_breakdown[syst_name] += cov_matrix;
                    }
                }

                Histogram final_total_mc_hist = result.getTotalHist();
                for(const auto& [syst_name, total_cov] : total_syst_breakdown){
                    final_total_mc_hist.addCovariance(total_cov);
                    result.addSystematic(syst_name, total_cov);
                }
                result.setTotalHist(final_total_mc_hist);

                for (const int channel_key : analysis_channel_keys) {
                    if (channel_key == 0) continue;
                    auto per_category_varied_hists = params_.systematics_controller.getAllVariedHistograms(channel_key, task_binning, category_scheme);
                    for(const auto& [syst_name, varied_hists] : per_category_varied_hists){
                        for(const auto& [var_name, hist] : varied_hists){
                            const auto& current_variations = result.getSystematicVariations();
                            if (current_variations.count(syst_name) && current_variations.at(syst_name).count(var_name)) {
                                result.addSystematicVariation(syst_name, var_name, current_variations.at(syst_name).at(var_name) + hist);
                            } else {
                                result.addSystematicVariation(syst_name, var_name, hist);
                            }
                        }
                    }
                }
            }

            if (params_.pot_scale_factor != 1.0) {
                result.scale(params_.pot_scale_factor);
            }

            analysis_phase_space[task_key] = result;
        }
        return analysis_phase_space;
    }
};

}

#endif