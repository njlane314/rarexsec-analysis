#ifndef ANALYSIS_WORKFLOW_H
#define ANALYSIS_WORKFLOW_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

#include "EventDisplay.h"
#include "PlotManager.h"
#include "SystematicsController.h"
#include "DataManager.h"

namespace AnalysisFramework {

class AnalysisWorkflow {
public:
    AnalysisWorkflow(const std::string& config_file,
                     const std::string& beam_key,
                     const std::vector<std::string>& runs_to_load,
                     bool blinded,
                     const std::string& analysis_channel_column = "inclusive_strange_channels",
                     const std::string& plot_output_dir = "plots")
        : data_manager_({.config_file = config_file,
                         .beam_key = beam_key,
                         .runs_to_load = runs_to_load,
                         .blinded = blinded,
                         .variable_options = {}}),
          systematics_controller_(data_manager_.getVariableManager()),
          analysis_channel_column_(analysis_channel_column),
          plot_manager_(plot_output_dir) {}

    AnalysisWorkflow& defineVariable(const std::string& name, const std::string& branch, const std::string& label, int n_bins, double low, double high, bool is_log = false, const std::string& short_label = "") {
        analysis_space_.defineVariable(name, branch, label, n_bins, low, high, is_log, short_label);
        return *this;
    }

    AnalysisWorkflow& defineVariable(const std::string& name, const std::string& branch, const std::string& label, const std::vector<double>& edges, bool is_log = false, const std::string& short_label = "") {
        analysis_space_.defineVariable(name, branch, label, edges, is_log, short_label);
        return *this;
    }

    AnalysisWorkflow& defineRegion(const std::string& name, const std::string& title, const TString& selection_key, const TString& preselection_key = "None", const std::string& short_title = "", const std::vector<std::string>& extra_queries_str = {}) {
        std::vector<TString> extra_queries_tstring;
        for (const auto& q_str : extra_queries_str) {
            extra_queries_tstring.push_back(TString(q_str.c_str()));
        }
        analysis_space_.defineRegion(name, title, selection_key, preselection_key, short_title, extra_queries_tstring);
        return *this;
    }

    AnalysisWorkflow& addWeightSystematic(const std::string& name) {
        systematics_controller_.addWeightSystematic(name);
        return *this;
    }

    AnalysisWorkflow& addUniverseSystematic(const std::string& name) {
        systematics_controller_.addUniverseSystematic(name);
        return *this;
    }

    AnalysisWorkflow& addDetectorSystematic(const std::string& name) {
        systematics_controller_.addDetectorSystematic(name);
        return *this;
    }

    AnalysisWorkflow& addNormaliseUncertainty(const std::string& name, double uncertainty) {
        systematics_controller_.addNormaliseUncertainty(name, uncertainty);
        return *this;
    }

    AnalysisWorkflow& loadAnalysisSpace(const std::string& space_name) {
        if (space_name == "muon") {
            defineVariable("muon_momentum", "selected_muon_momentum_range", "Muon Momentum [GeV]", 30, 0, 2);
            defineVariable("muon_length", "selected_muon_length", "Muon Length [cm]", 50, 0, 500);
            defineVariable("muon_cos_theta", "selected_muon_cos_theta", "Muon cos(#theta)", 40, -1, 1);

            defineRegion("numu_loose", "Loose NuMu Selection", "NUMU_CC_LOOSE", "QUALITY");
            defineRegion("numu_tight", "Tight NuMu Selection", "NUMU_CC_TIGHT", "QUALITY");
            defineRegion("track_score", "Track Score Selection", "TRACK_SCORE", "QUALITY");
            defineRegion("pid_score", "PID Score Selection", "PID_SCORE", "QUALITY");
            defineRegion("fiducial", "Fiducial Volume Selection", "FIDUCIAL_VOLUME", "QUALITY");
            defineRegion("track_length", "Track Length Selection", "TRACK_LENGTH", "QUALITY");
        } else {
            throw std::invalid_argument("Unknown analysis space: " + space_name);
        }
        return *this;
    }

    AnalysisPhaseSpace runAnalysis() {
        AnalysisRunner runner({
            .data_manager = data_manager_,
            .analysis_space = analysis_space_,
            .systematics_controller = systematics_controller_,
            .category_column = analysis_channel_column_
        });
        return runner.run();
    }

    void saveStackedPlot(const std::string& name, const AnalysisResult& result) {
        plot_manager_.saveStackedPlot(name, result, analysis_channel_column_);
    }

    void snapshotDataFrames(const std::string& selection_key,
                            const std::string& preselection_key,
                            const std::string& output_file,
                            const std::vector<std::string>& columns_to_save) {
        data_manager_.snapshotDataFrames(selection_key, preselection_key, output_file, columns_to_save);
    }

    void visualiseDetectorViews(const std::string& selection_key,
                                 const std::string& preselection_key,
                                 const std::string& additional_selection = "",
                                 int num_events = 1,
                                 int img_size = 512,
                                 const std::string& output_dir = "event_display_plots") {
        EventDisplay event_display(data_manager_, img_size, output_dir);
        event_display.visualiseDetectorViews(selection_key, preselection_key, additional_selection, num_events);
    }

    void visualiseSemanticViews(const std::string& selection_key,
                                 const std::string& preselection_key,
                                 const std::string& additional_selection = "",
                                 int num_events = 1,
                                 int img_size = 512,
                                 const std::string& output_dir = "event_display_plots") {
        EventDisplay event_display(data_manager_, img_size, output_dir);
        event_display.visualiseSemanticViews(selection_key, preselection_key, additional_selection, num_events);
    }

    const std::string& getAnalysisChannelColumn() const {
        return analysis_channel_column_;
    }

private:
    DataManager data_manager_;
    AnalysisSpace analysis_space_;
    SystematicsController systematics_controller_;
    PlotManager plot_manager_;
    std::string analysis_channel_column_;
};

} 

#endif // ANALYSIS_WORKFLOW_H