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

        /*AnalysisFramework::BDTManager bdt_manager;
        std::string bdt_model_output_path = "dataset/weights/TMVAClassification_BDT.weights.xml";
        std::string bdt_method_name = "BDT";
        std::string bdt_score_column_name = "bdt_score";
        std::vector<std::string> feature_column_names = {
            "nhits_u", 
            "nhits_v",
            "nhits_w", 
            "nclusters_u",
            "nclusters_v",
            "nclusters_w"
        };
       
        for (auto& [sample_key, sample_info] : data_manager.getAllSamplesMutable()) {
            if (sample_info.isMonteCarlo()) {
                ROOT::RDF::RNode df = sample_info.getDataFrame();
                df = bdt_manager.addBDTScoreColumn(df, bdt_score_column_name, bdt_model_output_path, bdt_method_name, feature_column_names);
                sample_info.setDataFrame(df);
            }
        }*/

        AnalysisFramework::AnalysisSpace analysis_space;
        analysis_space
            .defineVariable("opfilter_pe_beam", "_opfilter_pe_beam", "Beam OpFilter PE", 100, 0.0, 100.0)
            .defineVariable("topolotical_score", "nu_slice_topo_score", "Topological Score", 100, 0.0, 1.0)
            .defineVariable("nslice", "nslice", "Number of Slices", 3, 0, 3)
            .defineVariable("n_muon_candidates", "n_muon_candidates", "Number of Muon Candidates", 5, 0, 5)
            .defineVariable("nhits_u", "nhits_u", "Plane-U Hits", 90, 0, 5000)
            .defineVariable("nhits_v", "nhits_v", "Plane-V Hits", 90, 0, 5000)
            .defineVariable("nhits_w", "nhits_w", "Plane-W Hits", 90, 0, 5000)
            .defineVariable("charge_u", "charge_u", "Plane-U Charge", 80, 0, 400000)
            .defineVariable("charge_v", "charge_v", "Plane-V Charge", 80, 0, 400000)
            .defineVariable("charge_w", "charge_w", "Plane-W Charge", 80, 0, 400000)
            .defineVariable("wirerange_u", "wirerange_u", "Plane-U Wire Range", 60, 0, 4000.0)
            .defineVariable("timerange_u", "timerange_u", "Plane-U Time Range", 80, 0, 8000.0)
            .defineVariable("npfps", "n_pfps", "Number of PFPs", 15, 0, 15)
            .defineVariable("ntracks", "n_tracks", "Number of Tracks", 10, 0, 10)
            .defineVariable("nshowers", "n_showers", "Number of Showers", 10, 0, 10)
            .defineVariable("evnhits", "evnhits", "Event Hits", 100, 0, 50000)
            .defineVariable("slclustfrac", "slclustfrac", "Slice Cluster Fraction", 100, 0.0, 1.0)
            .defineVariable("selected", "selected", "Slice Selected", 2, 0, 2)
            .defineVariable("nu_e", "nu_e", "Neutrino Energy", 80, 0.0, 12.0)
            .defineVariable("semantic_salience", "semantic_salience", "n_{hits, slice}^{2}/n_{hits, event}", 60, 0.0, 2000.0)
            .defineVariable("nclusters_u", "nclusters_u", "Plane-U Clusters", 10, 0, 10)
            .defineVariable("nclusters_v", "nclusters_v", "Plane-V Clusters", 10, 0, 10)
            .defineVariable("nclusters_w", "nclusters_w", "Plane-W Clusters", 10, 0, 10)
            //.defineVariable("bdt_score", "bdt_score", "BDT Score", 100, -1.0, 1.0) 
            .defineVariable("has_muon_candidate", "has_muon_candidate", "Muon Candidate Selection", 2, 0, 2)
            .defineVariable("is_1p0pi", "is_1p0pi", "1 Proton 0 Pion Selection", 2, 0, 2)
            .defineVariable("is_Np0pi", "is_Np0pi", "N Protons 0 Pions Selection", 2, 0, 2)
            .defineVariable("is_1pi", "is_1pi", "1 Pion Selection", 2, 0, 2)
            .defineVariable("is_0p0pi", "is_0p0pi", "0 Protons 0 Pions Selection", 2, 0, 2)
            .defineVariable("is_1p1pi", "is_1p1pi", "1 Proton 1 Pion Selection", 2, 0, 2)
            .defineVariable("n_reco_protons", "n_reco_protons", "Number of Reconstructed Protons", 10, 0, 10)
            .defineVariable("n_reco_pions", "n_reco_pions", "Number of Reconstructed Pions", 10, 0, 10)
            .defineVariable("n_pfp_gen_0", "n_pfp_gen_0", "Number of PFPs Gen 0", 10, 0, 10)
            .defineVariable("n_pfp_gen_1", "n_pfp_gen_1", "Number of PFPs Gen 1", 10, 0, 10)
            .defineVariable("n_pfp_gen_2", "n_pfp_gen_2", "Number of PFPs Gen 2", 10, 0, 10)
            .defineVariable("n_pfp_gen_3", "n_pfp_gen_3", "Number of PFPs Gen 3", 10, 0, 10)
            .defineVariable("n_pfp_gen_4", "n_pfp_gen_4", "Number of PFPs Gen 4", 10, 0, 10)
            .defineVariable("selected_muon_cos_theta", "selected_muon_cos_theta", "Selected Muon Cosine Theta", 80, -1.0, 1.0)
            .defineRegion("ALL_EVENTS_REGION", "Empty Selection", {"ALL_EVENTS"})
            .defineRegion("QUALITY_REGION", "Quality Slice Presel.", {"QUALITY"})
            .defineRegion("SIGNAL_REGION", "Signal Region", {"QUALITY", "SIGNAL"})
            .defineRegion("NUMU_CC_REGION", "NuMu CC Selection", {"QUALITY", "NUMU_CC"})
            .defineRegion("NUMU_CC_VETO_1P0PI_REGION", "NuMu CC with 1p0pi Veto", {"QUALITY", "NUMU_CC", "VETO_1P0PI"})
            .defineRegion("NUMU_CC_VETO_NP0PI_REGION", "NuMu CC with Np0pi Veto", {"QUALITY", "NUMU_CC", "VETO_NP0PI"})
            .defineRegion("NUMU_CC_VETO_1PI_REGION", "NuMu CC with 1pi Veto", {"QUALITY", "NUMU_CC", "VETO_1PI"})
            .defineRegion("NUMU_CC_MULTI_PROTON_VETO_REGION", "NuMu CC with 1p0pi and Np0pi Veto", {"QUALITY", "NUMU_CC", "VETO_1P0PI", "VETO_NP0PI"})
            .defineRegion("NUMU_CC_SINGLE_PION_VETO_REGION", "NuMu CC with 1p0pi, Np0pi and 1pi Veto", {"QUALITY", "NUMU_CC", "VETO_1P0PI", "VETO_NP0PI", "VETO_1PI"})
            .defineRegion("NUMU_CC_SINGLE_PROTON_VETO_REGION", "NuMu CC with 1p0pi, Np0pi, 0p1pi and 1p1pi Veto", {"QUALITY", "NUMU_CC", "VETO_1P0PI", "VETO_NP0PI", "VETO_1PI", "VETO_1P1PI"});

        AnalysisFramework::SystematicsController systematics_controller(data_manager.getVariableManager());

        AnalysisFramework::AnalysisRunner runner({
            .data_manager = data_manager,
            .analysis_space = analysis_space,
            .systematics_controller = systematics_controller,
            .category_column = "inclusive_strange_channels"
        });

        AnalysisFramework::AnalysisPhaseSpace analysis_phase_space = runner.run();

        AnalysisFramework::PlotManager plot_manager;
        /*plot_manager.saveStackedPlot(analysis_phase_space, "opfilter_pe_beam", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "topolotical_score", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nslice", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_muon_candidates", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_u", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_v", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_w", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_u", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_v", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_w", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "opfilter_pe_beam", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "topolotical_score", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nslice", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_muon_candidates", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_v", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_w", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_v", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "charge_w", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "wirerange_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "timerange_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "npfps", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "evnhits", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "slclustfrac", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "evnhits", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "slclustfrac", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "selected", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "selected", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nu_e", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nu_e", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nu_e", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "semantic_salience", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "semantic_salience", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_u", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_v", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_w", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_u", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_v", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_w", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_u", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_v", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nclusters_w", "SIGNAL_REGION", "inclusive_strange_channels");
        //plot_manager.saveStackedPlot(analysis_phase_space, "bdt_score", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        //plot_manager.saveStackedPlot(analysis_phase_space, "bdt_score", "QUALITY_REGION", "inclusive_strange_channels");
        //plot_manager.saveStackedPlot(analysis_phase_space, "bdt_score", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "has_muon_candidate", "ALL_EVENTS_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "has_muon_candidate", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_u", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_v", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nhits_w", "NUMU_CC_REGION", "inclusive_strange_channels");
        //plot_manager.saveStackedPlot(analysis_phase_space, "bdt_score", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "selected_muon_cos_theta", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "selected_muon_cos_theta", "QUALITY_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "selected_muon_cos_theta", "SIGNAL_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "npfps", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_0", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_1", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_2", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_3", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_4", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1p0pi", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_Np0pi", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1pi", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_0p0pi", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_protons", "NUMU_CC_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_pions", "NUMU_CC_REGION", "inclusive_strange_channels");*/

        plot_manager.saveStackedPlot(analysis_phase_space, "is_1p0pi", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1p0pi", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1p0pi", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_Np0pi", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_Np0pi", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_Np0pi", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1pi", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1pi", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1pi", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_protons", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_pions", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_protons", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_pions", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_protons", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_pions", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "is_Np0pi", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "is_1pi", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_protons", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_reco_pions", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "is_1p1pi", "NUMU_CC_SINGLE_PION_VETO_REGION", "inclusive_strange_channels");
    
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_2", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_3", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_2", "NUMU_CC_SINGLE_PION_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_3", "NUMU_CC_SINGLE_PION_VETO_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_2", "NUMU_CC_SINGLE_PROTON_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "n_pfp_gen_3", "NUMU_CC_SINGLE_PROTON_VETO_REGION", "inclusive_strange_channels");

        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_SINGLE_PION_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "nshowers", "NUMU_CC_SINGLE_PROTON_VETO_REGION", "inclusive_strange_channels");
        
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_VETO_1P0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_VETO_NP0PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_VETO_1PI_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_MULTI_PROTON_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_SINGLE_PION_VETO_REGION", "inclusive_strange_channels");
        plot_manager.saveStackedPlot(analysis_phase_space, "ntracks", "NUMU_CC_SINGLE_PROTON_VETO_REGION", "inclusive_strange_channels");    

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}