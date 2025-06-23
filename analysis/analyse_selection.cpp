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
            .defineVariable("constant_0_5", "constant_0_5", "Constant Value 0.5", 1, 0.0, 1.0)
            .defineVariable("nu_e", "nu_e", "True Neutrino Energy", 60, 0.0, 12.0)
            .defineVariable("true_nu_vtx_x", "true_nu_vtx_x", "True Neutrino Vertex X [cm]", 70, -30, 290)
            .defineVariable("true_nu_vtx_y", "true_nu_vtx_y", "True Neutrino Vertex Y [cm]", 70, -150, 150)
            .defineVariable("true_nu_vtx_z", "true_nu_vtx_z", "True Neutrino Vertex Z [cm]", 60, -30, 1070)
            .defineVariable("_opfilter_pe_beam", "_opfilter_pe_beam", "Optical Filter Beam", 100, 0.0, 1000.0)
            .defineVariable("selected", "selected", "Slice Selected", 2, -0.5, 1.5)
            .defineVariable("nslice", "nslice", "Number of Slices", 3, 0.0, 3.0)
            .defineVariable("topological_score", "topological_score", "Topological Score", 100, 0.0, 1.0)
            .defineVariable("reco_nu_vtx_sce_x", "reco_nu_vtx_sce_x", "Reconstructed Neutrino Vertex X [cm]", 70, -30, 290)
            .defineVariable("reco_nu_vtx_sce_y", "reco_nu_vtx_sce_y", "Reconstructed Neutrino Vertex Y [cm]", 70, -150, 150)
            .defineVariable("reco_nu_vtx_sce_z", "reco_nu_vtx_sce_z", "Reconstructed Neutrino Vertex Z [cm]", 70, -30, 1070)
            .defineVariable("quality_selector", "quality_selector", "Slice Quality Selector", 2, -0.5, 1.5)
            .defineRegion("ALL_EVENTS", "Empty Selection", {"ALL_EVENTS"});

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

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "constant_0_5", "ALL_EVENTS", "exclusive_strange_channels", false, {}, false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "constant_0_5", "ALL_EVENTS", "inclusive_strange_channels", false, {}, false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "nu_e", "ALL_EVENTS", "exclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nu_e", "ALL_EVENTS", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "true_nu_vtx_x", "ALL_EVENTS", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "true_nu_vtx_y", "ALL_EVENTS", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "true_nu_vtx_z", "ALL_EVENTS", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "_opfilter_pe_beam", "ALL_EVENTS", "inclusive_strange_channels", true, {{20.0, AnalysisFramework::CutDirection::GreaterThan}});  
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "selected", "ALL_EVENTS", "inclusive_strange_channels", true, {{0.5, AnalysisFramework::CutDirection::GreaterThan}, {1.5, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nslice", "ALL_EVENTS", "inclusive_strange_channels", true, {{1.0, AnalysisFramework::CutDirection::GreaterThan}, {2.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "topological_score", "ALL_EVENTS", "inclusive_strange_channels", true, {{0.2, AnalysisFramework::CutDirection::GreaterThan}, {1.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "reco_nu_vtx_sce_x", "ALL_EVENTS", "inclusive_strange_channels", true, {{5.0, AnalysisFramework::CutDirection::GreaterThan}, {251.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "reco_nu_vtx_sce_y", "ALL_EVENTS", "inclusive_strange_channels", true, {{-110.0, AnalysisFramework::CutDirection::GreaterThan}, {110.0, AnalysisFramework::CutDirection::LessThan}});
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "reco_nu_vtx_sce_z", "ALL_EVENTS", "inclusive_strange_channels", true, {{20.0, AnalysisFramework::CutDirection::GreaterThan}, {986.0, AnalysisFramework::CutDirection::LessThan}});

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "quality_selector", "ALL_EVENTS", "inclusive_strange_channels", true, {{0.5, AnalysisFramework::CutDirection::GreaterThan}, {1.5, AnalysisFramework::CutDirection::LessThan}});

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}