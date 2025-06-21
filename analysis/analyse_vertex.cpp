#include <iostream>
#include <string>
#include <vector>
#include <map>

#include "ROOT/RDataFrame.hxx"

#include "AnalysisFramework.h"

int main() {
    try {
        //ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {}
        });

        /*AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("nhits_u", "nhits_u", "Plane-U Hits", 90, 0, 5000)
            .defineVariable("nhits_v", "nhits_v", "Plane-V Hits", 90, 0, 5000)
            .defineVariable("nhits_w", "nhits_w", "Plane-W Hits", 90, 0, 5000)
            .defineVariable("reco_nu_vtx_sce_x", "reco_nu_vtx_sce_x", "Reconstructed Neutrino Vertex X [cm]", 70, -30, 290)
            .defineVariable("reco_nu_vtx_sce_y", "reco_nu_vtx_sce_y", "Reconstructed Neutrino Vertex Y [cm]", 70, -150, 150)
            .defineVariable("reco_nu_vtx_sce_z", "reco_nu_vtx_sce_z", "Reconstructed Neutrino Vertex Z [cm]", 70, -30, 1070)
            .defineVariable("true_nu_vtx_x", "true_nu_vtx_x", "True Neutrino Vertex X [cm]", 70, -30, 290)
            .defineVariable("true_nu_vtx_y", "true_nu_vtx_y", "True Neutrino Vertex Y [cm]", 70, -150, 150)
            .defineVariable("true_nu_vtx_z", "true_nu_vtx_z", "True Neutrino Vertex Z [cm]", 70, -30, 1070)
            .defineVariable("npfps", "n_pfps", "Number of PFPs", 15, 0, 15)
            .defineVariable("ntracks", "n_tracks", "Number of Tracks", 10, 0, 10)
            .defineVariable("nshowers", "n_showers", "Number of Showers", 10, 0, 10)
            .defineRegion("ALL_EVENTS_REGION", "Empty Selection", {"ALL_EVENTS"})
            .defineRegion("ZERO_HITS_PLANE_REGION", "Zero Hits in a Plane", {"ZERO_HITS_PLANE"})
            .defineRegion("ZERO_HITS_COLLECTION_PLANE_REGION", "Zero Hits in Collection Plane", {"ZERO_HITS_COLLECTION_PLANE"})
            .defineRegion("QUALITY_REGION", "Quality Slice Presel.", {"QUALITY"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .category_column = "inclusive_strange_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_phase_space = runner.run();

        AnalysisFramework::PlotManager plot_manager;
        
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_u", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_v", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_w", "ALL_EVENTS_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_v", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_w", "QUALITY_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_x", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_y", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_z", "ALL_EVENTS_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_x", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_y", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_z", "QUALITY_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_x", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_y", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "reco_nu_vtx_sce_z", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_x", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_y", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_z", "ZERO_HITS_PLANE_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_x", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_y", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "true_nu_vtx_z", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "npfps", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "ZERO_HITS_COLLECTION_PLANE_REGION", "inclusive_strange_channels");*/
        
        AnalysisFramework::EventDisplay event_display(data_manager, 512, "plots");
        event_display.visualiseDetectorViews(
            "ZERO_HITS_PLANE", 
            "NONE",            
            "inclusive_strange_channels == 10 || inclusive_strange_channels == 11",                
            5                  
        );

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}