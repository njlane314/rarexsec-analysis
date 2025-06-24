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
            .variable_options = {
                .load_showers = true,
                .load_blips = true}
        });

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
        .defineVariable("n_blips", "n_blips", "Number of Blips", 800, 0, 800)
        .defineVariable("total_blip_energy", "total_blip_energy", "Total Blip Energy [MeV]", 100, 0.0, 500.0)
        .defineVariable("max_blip_energy", "max_blip_energy", "Max Blip Energy [MeV]", 100, 0.0, 500.0)
        .defineVariable("n_valid_blips", "n_valid_blips", "Number of Valid Blips", 800, 0, 800)
        .defineVariable("blip_dist_from_vtx", "blip_dist_from_vtx", "Blip Distance from Vertex [cm]", 1000, 0, 1000)
        .defineVariable("n_blips_near_vtx", "n_blips_near_vtx", "Number of Blips Near Vertex (within 10 cm)", 100, 0, 100)
        .defineVariable("nearby_blip_energy_fraction", "nearby_blip_energy_fraction", "Fraction of Total Blip Energy from Nearby Blips", 100, 0.0, 1.0)
        .defineVariable("n_neutron_blips_truth", "n_neutron_blips_truth", "Number of Neutron Blips (Truth)", 100, 0, 100)
        .defineVariable("n_blips_prox_track", "n_blips_prox_track", "Number of Blips Proximal to Tracks (< 5 cm)", 100, 0, 100)
        .defineVariable("n_blips_halo_30_50cm", "n_blips_halo_30_50cm", "Number of Blips in Halo (30-50 cm)", 100, 0, 100)
        .defineVariable("blip_exp_moment_10cm", "blip_exp_moment_10cm", "Blip Energy Concentration, 10 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_12cm", "blip_exp_moment_12cm", "Blip Energy Concentration, 12 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_15cm", "blip_exp_moment_15cm", "Blip Energy Concentration, 15 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_20cm", "blip_exp_moment_20cm", "Blip Energy Concentration, 20 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_30cm", "blip_exp_moment_30cm", "Blip Energy Concentration, 30 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_40cm", "blip_exp_moment_40cm", "Blip Energy Concentration, 40 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_50cm", "blip_exp_moment_50cm", "Blip Energy Concentration, 50 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_100cm", "blip_exp_moment_100cm", "Blip Energy Concentration, 100 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_18cm", "blip_exp_moment_18cm", "Blip Energy Concentration, 18 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_22cm", "blip_exp_moment_22cm", "Blip Energy Concentration, 22 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineVariable("blip_exp_moment_25cm", "blip_exp_moment_25cm", "Blip Energy Concentration, 25 cm Length-Scale [MeV]", 100, 0.0, 20.0)
        .defineRegion("NUMU_CC", "NuMu CC Selection", {"QUALITY", "NUMU_CC"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner_inclusive({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .event_category_column = "inclusive_strange_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_inclusive_phase_space = runner_inclusive.run();

        AnalysisFramework::PlotManager plot_manager;

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_blips", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_blip_energy", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "max_blip_energy", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_valid_blips", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_dist_from_vtx", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_blips_near_vtx", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nearby_blip_energy_fraction", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_neutron_blips_truth", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_blips_prox_track", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_blips_halo_30_50cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_10cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_12cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_15cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_20cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_30cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_40cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_50cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_100cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_18cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_22cm", "NUMU_CC", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "blip_exp_moment_25cm", "NUMU_CC", "inclusive_strange_channels");
        
    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}