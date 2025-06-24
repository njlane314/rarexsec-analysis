#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "ROOT/RDataFrame.hxx"

#include "AnalysisFramework.h"

int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {}
        });

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("trk_len_v", "trk_len_v", "Track Length [cm]", 170, 0.0, 100.0, false, "", true)
            .defineVariable("trk_llr_pid_score_v", "trk_llr_pid_score_v", "Track LLR PID Score", 150, -1.0, 1.0, false, "", true)
            .defineVariable("trk_score_v", "trk_score_v", "Track Score", 100, 0.0, 1.0, false, "", true)
            .defineVariable("trk_nhits_u_v_float", "trk_nhits_u_v_float", "Track Hits U", 100, 0.0, 200.0, false, "", true)
            .defineVariable("trk_trunk_dEdx_y_v", "trk_trunk_dEdx_y_v", "Track Trunk dEdx Y", 100, 0.0, 10.0, false, "", true) 
            .defineVariable("trk_trunk_rr_dEdx_y_v", "trk_trunk_rr_dEdx_y_v", "Track Trunk RR dEdx Y", 200, 0.0, 10.0, false, "", true)
            .defineVariable("trk_end_spacepoints_v_float", "trk_end_spacepoints_v_float", "Track End Spacepoints", 100, 0.0, 200.0, false, "", true)
            .defineVariable("trk_distance_v", "trk_distance_v", "Track Distance [cm]", 100, 0.0, 15.0, false, "", true)
            .defineVariable("n_muons", "n_muons", "Number of Muons", 5, 0, 5)
            .defineVariable("n_pfps", "n_pfps", "Number of PFParticles", 10, 0, 10)
            .defineVariable("muon_candidate_selector", "muon_candidate_selector", "Muon Candidate Selector", 2, -0.5, 1.5)
            .defineVariable("trk_trunk_rr_dEdx_avg_v", "trk_trunk_rr_dEdx_avg_v", "Track Trunk RR Avg dEdx", 200, 0.0, 10.0, false, "", true)
            .defineVariable("n_tracks", "n_tracks", "Number of Tracks", 10, 0, 10)
            .defineVariable("n_showers", "n_showers", "Number of Showers", 10, 0, 10)
            .defineVariable("charge_u", "charge_u", "Charge U Plane", 100, 0.0, 10000.0, false, "", true)
            .defineRegion("QUALITY", "Quality Slice Pres.", {"QUALITY"})
            .defineRegion("ALL_EVENTS", "All Events", {"NONE"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner_inclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "inclusive_strange_channels",
            .particle_category_column = "backtracked_pdg",     
            .particle_category_scheme = "particle_pdg_channels"
        });

        AnalysisFramework::AnalysisRunner runner_exclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "exclusive_strange_channels",
            .particle_category_column = "backtracked_pdg",     
            .particle_category_scheme = "particle_pdg_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_inclusive_phase_space = runner_inclusive.run();
        AnalysisFramework::AnalysisPhaseSpace analysis_exclusive_phase_space = runner_exclusive.run();

        AnalysisFramework::PlotManager plot_manager;

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "QUALITY", "particle_pdg_channels", false, {{5.0, AnalysisFramework::CutDirection::GreaterThan}});
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_llr_pid_score_v", "QUALITY", "particle_pdg_channels", false, {{-0.2, AnalysisFramework::CutDirection::GreaterThan}, {1.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_score_v", "QUALITY", "particle_pdg_channels", false, {{0.3, AnalysisFramework::CutDirection::GreaterThan}, {1.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_nhits_u_v_float", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_dEdx_y_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_y_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_end_spacepoints_v_float", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_distance_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_muons", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pfps", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_avg_v", "QUALITY", "particle_pdg_channels", false, {{0.0, AnalysisFramework::CutDirection::GreaterThan}, {3.0, AnalysisFramework::CutDirection::LessThan}});
        
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "muon_candidate_selector", "QUALITY", "inclusive_strange_channels", true, {{0.5, AnalysisFramework::CutDirection::GreaterThan}, {1.5, AnalysisFramework::CutDirection::LessThan}});
    
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}