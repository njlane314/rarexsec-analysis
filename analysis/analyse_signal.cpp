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
            .defineVariable("nu_e", "nu_e", "True Neutrino Energy", 60, 0.0, 12.0)
            .defineVariable("trk_len_v", "trk_len_v", "Track Length (cm)", 200, 0.0, 200.0, false, "", true)
            .defineVariable("trk_llr_pid_score_v", "trk_llr_pid_score_v", "Track LLR PID Score", 200, -1.0, 1.0, false, "", true)
            .defineVariable("trk_pida_v", "trk_pida_v", "Track PIDA", 200, 0.0, 30.0, false, "", true)
            .defineVariable("trk_score_v", "trk_score_v", "Track Score", 200, 0.0, 1.0, false, "", true)
            .defineVariable("trk_nhits_u_v_float", "trk_nhits_u_v_float", "Track Hits U", 200, 0.0, 200.0, false, "", true)
            .defineVariable("trk_pid_chimu_v", "trk_pid_chimu_v", "Track PID ChiMu", 100, 0, 500.0, false, "", true)
            .defineVariable("trk_pid_chipr_v", "trk_pid_chipr_v", "Track PID ChiPr", 100, 0, 500.0, false, "", true)
            .defineVariable("trk_pid_chika_v", "trk_pid_chika_v", "Track PID ChiKa", 100, 0, 500.0, false, "", true)
            .defineVariable("trk_pid_chipi_v", "trk_pid_chipi_v", "Track PID ChiPi", 100, 0, 500.0, false, "", true)
            .defineVariable("trk_trunk_dEdx_y_v", "trk_trunk_dEdx_y_v", "Track Trunk dEdx Y", 200, 0.0, 10.0, false, "", true) 
            .defineVariable("trk_trunk_rr_dEdx_y_v", "trk_trunk_rr_dEdx_y_v", "Track Trunk RR dEdx Y", 200, 0.0, 10.0, false, "", true)
            .defineVariable("trk_end_spacepoints_v_float", "trk_end_spacepoints_v_float", "Track End Spacepoints", 100, 0.0, 200.0, false, "", true)
            .defineVariable("trk_distance_v", "trk_distance_v", "Track Distance (cm)", 150, 0.0, 15.0, false, "", true)
            /*.defineVariable("trk_avg_deflection_stdev_v", "trk_avg_deflection_stdev_v", "Track Avg Deflection Stdev", 100, 0.0, 1.0, false, "", true)
            .defineVariable("trk_avg_deflection_mean_v", "trk_avg_deflection_mean_v", "Track Avg Deflection Mean", 100, 0.0, 0.0, false, "", true)
            .defineVariable("trk_avg_deflection_separation_mean_v", "trk_avg_deflection_separation_mean_v", "Track Avg Deflection Separation Mean", 100, 0.0, 1.0, false, "", true)*/
            .defineVariable("n_muons", "n_muons", "Number of Muons", 5, 0, 5)
            .defineVariable("n_pfps", "n_pfps", "Number of PFParticles", 10, 0, 10)
            .defineVariable("n_protons", "n_protons", "Number of Protons", 5, 0, 5)
            .defineVariable("n_pions", "n_pions", "Number of Pions", 5, 0, 5)
            .defineVariable("n_electrons", "n_electrons", "Number of Electrons", 5, 0, 5)
            .defineVariable("hadronic_E", "hadronic_E", "Hadronic Energy (GeV)", 100, 0.0, 10.0)
            .defineVariable("hadronic_W", "hadronic_W", "Hadronic Invariant Mass (GeV)", 100, 0.0, 10.0)
            .defineVariable("total_E_visible_numu", "total_E_visible_numu", "Total Visible Energy (GeV)", 100, 0.0, 10.0)
            .defineVariable("missing_pT_numu", "missing_pT_numu", "Missing Transverse Momentum (GeV/c)", 100, 0.0, 10.0)
            .defineVariable("muon_E_fraction", "muon_E_fraction", "Muon Energy Fraction", 100, -1.0, 1.0)   
            .defineVariable("1mu0p0pi", "is_1mu0p0pi", "1 Muon, 0 Protons, 0 Pions", 2, 0, 1)
            .defineVariable("1mu1p0pi", "is_1mu1p0pi", "1 Muon, 1 Proton, 0 Pions", 2, 0, 1)
            .defineVariable("1muNp0pi", "is_1muNp0pi", "1 Muon, N Protons, 0 Pions", 2, 0, 1)
            .defineVariable("1mu0p1pi", "is_1mu0p1pi", "1 Muon, 0 Protons, 1 Pion", 2, 0, 1)
            .defineVariable("1mu1p1pi", "is_1mu1p1pi", "1 Muon, 1 Proton, 1 Pion", 2, 0, 1)
            .defineVariable("1mu_other", "is_1mu_other", "1 Muon, Other", 2, 0, 1)
            .defineVariable("1e0p", "is_1e0p", "1 Electron, 0 Protons", 2, 0, 1)
            .defineVariable("is_nc_like", "is_nc_like", "NC-like Event", 2, 0, 1)   
            .defineVariable("1e1p", "is_1e1p", "1 Electron, 1 Proton", 2, 0, 1)
            .defineVariable("1eNp", "is_1eNp", "1 Electron, N Protons", 2, 0, 1)
            .defineVariable("is_mulit_mu", "is_mulit_mu", "Multi-Muon Event", 2, 0, 1)
            .defineVariable("trk_bragg_p_v", "trk_bragg_p_v", "Track Bragg Peak (GeV)", 100, 0.0, 1.0, false, "", true)
            .defineVariable("trk_bragg_mu_v", "trk_bragg_mu_v", "Track Bragg Peak (GeV)", 100, 0.0, 1.0, false, "", true)
            .defineVariable("trk_bragg_pion_v", "trk_bragg_pion_v", "Track Bragg Peak (GeV)", 100, 0.0, 1.0, false, "", true)
            .defineVariable("trk_calo_energy_u_v", "trk_calo_energy_u_v", "Track Calorimetry Energy U (GeV)", 100, 0.0, 1000.0, false, "", true)
            .defineVariable("trk_calo_energy_v_v", "trk_calo_energy_v_v", "Track Calorimetry Energy V (GeV)", 100, 0.0, 1000.0, false, "", true)
            .defineVariable("trk_calo_energy_y_v", "trk_calo_energy_y_v", "Track Calorimetry Energy Y (GeV)", 100, 0.0, 1000.0, false, "", true)
            .defineRegion("ALL_EVENTS", "Empty Selection", {"ALL_EVENTS"})
            .defineRegion("QUALITY", "Quality Slice Pres.", {"QUALITY"})
            .defineRegion("SIGNAL", "Signal Filter", {"SIGNAL"})
            .defineRegion("NUMU_CC_SEL", "NuMu CC Selection", {"QUALITY", "NUMU_CC"});

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

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nu_e", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "nu_e", "ALL_EVENTS", "exclusive_strange_channels", true);   
       
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "nu_e", "SIGNAL", "inclusive_strange_channels", false); 
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "nu_e", "SIGNAL", "exclusive_strange_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_len_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_llr_pid_score_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_llr_pid_score_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_llr_pid_score_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_llr_pid_score_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_pida_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_pida_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_pida_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_pida_v", "NUMU_CC_SEL", "particle_pdg_channels", false);  

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_score_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_score_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_score_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_score_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_nhits_u_v_float", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_nhits_u_v_float", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_nhits_u_v_float", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_nhits_u_v_float", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chimu_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chimu_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chimu_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chimu_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_p_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_p_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_p_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_p_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_mu_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_mu_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_mu_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_mu_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_pion_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_pion_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_pion_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_bragg_pion_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_u_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_u_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_u_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_u_v", "NUMU_CC_SEL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_v_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_v_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_v_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_v_v", "NUMU_CC_SEL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_y_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_y_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_y_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_calo_energy_y_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipr_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipr_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipr_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipr_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chika_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chika_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chika_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chika_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipi_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipi_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipi_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_exclusive_phase_space, "trk_pid_chipi_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_dEdx_y_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_dEdx_y_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_dEdx_y_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_dEdx_y_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_y_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_y_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_y_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_trunk_rr_dEdx_y_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_end_spacepoints_v_float", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_end_spacepoints_v_float", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_end_spacepoints_v_float", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_end_spacepoints_v_float", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_distance_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_distance_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_distance_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_distance_v", "NUMU_CC_SEL", "particle_pdg_channels", false);    

        /*plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_stdev_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_stdev_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_stdev_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_stdev_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_mean_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_mean_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_mean_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_mean_v", "NUMU_CC_SEL", "particle_pdg_channels", false);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_separation_mean_v", "ALL_EVENTS", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_separation_mean_v", "SIGNAL", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_separation_mean_v", "QUALITY", "particle_pdg_channels", false);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "trk_avg_deflection_separation_mean_v", "NUMU_CC_SEL", "particle_pdg_channels", false);*/

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_muons", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_muons", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_muons", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_muons", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pfps", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pfps", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pfps", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pfps", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_protons", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_protons", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_protons", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_protons", "NUMU_CC_SEL", "inclusive_strange_channels", true);   

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pions", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pions", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pions", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_pions", "NUMU_CC_SEL", "inclusive_strange_channels", true); 

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_electrons", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_electrons", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_electrons", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "n_electrons", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_E", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_E", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_E", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_E", "NUMU_CC_SEL", "inclusive_strange_channels", true);      

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_W", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_W", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_W", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "hadronic_W", "NUMU_CC_SEL", "inclusive_strange_channels", true);  

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_E_visible_numu", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_E_visible_numu", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_E_visible_numu", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "total_E_visible_numu", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "missing_pT_numu", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "missing_pT_numu", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "missing_pT_numu", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "missing_pT_numu", "NUMU_CC_SEL", "inclusive_strange_channels", true); 

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "muon_E_fraction", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "muon_E_fraction", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "muon_E_fraction", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "muon_E_fraction", "NUMU_CC_SEL", "inclusive_strange_channels", true); 

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p0pi", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p0pi", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p0pi", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p0pi", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p0pi", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p0pi", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p0pi", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p0pi", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1muNp0pi", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1muNp0pi", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1muNp0pi", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1muNp0pi", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p1pi", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p1pi", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p1pi", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu0p1pi", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p1pi", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p1pi", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p1pi", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu1p1pi", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu_other", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu_other", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu_other", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1mu_other", "NUMU_CC_SEL", "inclusive_strange_channels", true);   

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e0p", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e0p", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e0p", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e0p", "NUMU_CC_SEL", "inclusive_strange_channels", true);        

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_nc_like", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_nc_like", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_nc_like", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_nc_like", "NUMU_CC_SEL", "inclusive_strange_channels", true);  

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e1p", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e1p", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e1p", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1e1p", "NUMU_CC_SEL", "inclusive_strange_channels", true);

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1eNp", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1eNp", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1eNp", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "1eNp", "NUMU_CC_SEL", "inclusive_strange_channels", true);    

        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_mulit_mu", "ALL_EVENTS", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_mulit_mu", "SIGNAL", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_mulit_mu", "QUALITY", "inclusive_strange_channels", true);
        plot_manager.saveStackedPlot(analysis_inclusive_phase_space, "is_mulit_mu", "NUMU_CC_SEL", "inclusive_strange_channels", true);

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}