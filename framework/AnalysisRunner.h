#ifndef ANALYSIS_RUNNER_H
#define ANALYSIS_RUNNER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <variant>
#include <iostream>
#include <future>
#include <set>

#include "ROOT/RDataFrame.hxx"
#include "ROOT/RResultPtr.hxx"
#include "TH1.h"

#include "DataManager.h"
#include "AnalysisSpace.h"
#include "Binning.h"
#include "SystematicsController.h"
#include "AnalysisResult.h"
#include "ChannelManager.h"
#include "HistogramCategoriser.h"
#include "EventChannelCategoriser.h"
#include "ParticleTypeCategoriser.h"

namespace AnalysisFramework {

class AnalysisRunner {
public:
    explicit AnalysisRunner(DataManager& dm,
                              AnalysisSpace& as,
                              SystematicsController& sc,
                              ChannelManager& cm,
                              double pot_scale,
                              std::string event_cat_col,
                              std::string particle_cat_col)
        : data_manager_(dm),
          analysis_space_(as),
          systematics_controller_(sc),
          channel_manager_(cm),
          pot_scale_factor_(pot_scale),
          event_category_column_(std::move(event_cat_col)),
          particle_category_column_(std::move(particle_cat_col)) {}

    AnalysisPhaseSpace run() {
        this->generateTasks();
        this->bookHistograms();
        return this->processResults();
    }

private:
    DataManager& data_manager_;
    AnalysisSpace& analysis_space_;
    SystematicsController& systematics_controller_;
    ChannelManager& channel_manager_;
    double pot_scale_factor_;
    std::string event_category_column_;
    std::string particle_category_column_;

    std::vector<Binning> plot_tasks_;
    std::map<std::string, Binning> plot_task_map_;
    
    std::map<std::string, ROOT::RDF::RResultPtr<TH1D>> data_futures_;
    std::map<std::string, std::map<std::string, std::map<int, ROOT::RDF::RResultPtr<TH1D>>>> mc_category_futures_;

    void generateTasks() {
        const auto& variables = analysis_space_.getVariables();
        const auto& regions = analysis_space_.getRegions();

        for (const auto& [var_name, var_props] : variables) {
            for (const auto& [reg_name, reg_props] : regions) {
                Binning::Parameters p;
                p.variable = var_props.branch_expression.c_str();
                p.variable_tex = var_props.axis_label.c_str();
                p.selection_keys = reg_props.selection_keys;
                p.selection_tex = reg_props.title.c_str();

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
        const auto det_var_nodes = data_manager_.getAssociatedVariations();

        for (const auto& [task_key, task_binning] : plot_task_map_) {
            TH1D model(task_key.c_str(), task_key.c_str(), task_binning.nBins(), task_binning.bin_edges.data());

            for (const auto& [sample_key, sample_info] : data_manager_.getAllSamples()) {
                ROOT::RDF::RNode df = sample_info.getDataFrame();
                auto main_filtered_df = df.Filter(task_binning.selection_query.Data());

                if (sample_info.isMonteCarlo()) {
                    std::unique_ptr<HistogramCategoriser> categoriser;
                    if (task_binning.is_particle_level) {
                        categoriser = std::make_unique<ParticleTypeCategoriser>(particle_category_column_, channel_manager_);
                    } else {
                        categoriser = std::make_unique<EventChannelCategoriser>(event_category_column_, channel_manager_);
                    }
                    mc_category_futures_[task_key][sample_key] = categoriser->bookHistograms(main_filtered_df, task_binning, model);
                    
                    systematics_controller_.bookVariations(
                        task_key, sample_key, df, det_var_nodes, task_binning,
                        task_binning.selection_query.Data(), event_category_column_, event_category_column_);

                } else if (!data_manager_.isBlinded()){
                    data_futures_[task_key] = main_filtered_df.Histo1D(model, task_binning.variable.Data());
                }
            }
        }
    }

    AnalysisPhaseSpace processResults() {
        AnalysisPhaseSpace analysis_phase_space;

        for (const auto& [task_key, task_binning] : plot_task_map_) {
            AnalysisResult result;
            result.setBlinded(data_manager_.isBlinded());
            result.setPOT(data_manager_.getDataPOT());
            result.setBeamKey(data_manager_.getBeamKey());
            result.setRuns(data_manager_.getRunsToLoad());

            if (!result.isBlinded() && data_futures_.count(task_key)) {
                result.setDataHist(Histogram(task_binning, *data_futures_.at(task_key).GetPtr(), "data_hist", "Data"));
            }

            std::map<std::string, Histogram> total_mc_by_channel;
            for (const auto& [sample_key, sample_info] : data_manager_.getAllSamples()) {
                if (sample_info.isMonteCarlo()) {
                    std::unique_ptr<HistogramCategoriser> categoriser;
                    if (task_binning.is_particle_level) {
                        categoriser = std::make_unique<ParticleTypeCategoriser>(particle_category_column_, channel_manager_);
                    } else {
                        categoriser = std::make_unique<EventChannelCategoriser>(event_category_column_, channel_manager_);
                    }

                    auto processed_hists = categoriser->collectHistograms(mc_category_futures_.at(task_key).at(sample_key), task_binning);
                    for(const auto& [name, hist] : processed_hists) {
                        if (total_mc_by_channel.count(name)) {
                            total_mc_by_channel[name] = total_mc_by_channel[name] + hist;
                        } else {
                            total_mc_by_channel[name] = hist;
                        }
                    }
                }
            }

            Histogram total_mc_nominal(task_binning, "total_mc", "Total MC");
            for(const auto& [name, hist] : total_mc_by_channel){
                if (hist.sum() > 1e-6) {
                    result.addChannel(name, hist);
                    total_mc_nominal = total_mc_nominal + hist;
                }
            }
            result.setTotalHist(total_mc_nominal);
            
            if (!task_binning.is_particle_level) {
                const auto nominal_hist_for_cov = result.getTotalHist();
                std::map<std::string, TMatrixDSym> total_syst_breakdown;
                const auto channel_keys = channel_manager_.getChannelKeys(event_category_column_);

                for (const int channel_key : channel_keys) {
                    if (channel_key == 0) continue;
                    auto per_category_cov_map = systematics_controller_.computeAllCovariances(channel_key, nominal_hist_for_cov, task_binning, event_category_column_);
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

                for (const int channel_key : channel_keys) {
                    if (channel_key == 0) continue;
                    auto per_category_varied_hists = systematics_controller_.getAllVariedHistograms(channel_key, task_binning, event_category_column_);
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

            if (pot_scale_factor_ != 1.0) {
                result.scale(pot_scale_factor_);
            }
            analysis_phase_space[task_key] = result;
        }
        return analysis_phase_space;
    }
};

}

#endif