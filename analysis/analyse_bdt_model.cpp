#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <numeric>
#include <thread>

#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include "TGraph.h"
#include "TH1D.h"
#include "TLatex.h"
#include "TStyle.h"

#include "AnalysisFramework.h"

int main() {
    try {
        // Enable ROOT's implicit multi-threading
        ROOT::EnableImplicitMT();

        // Initialize DataManager
        AnalysisFramework::DataManager data_manager({
            .config_file = "/exp/uboone/app/users/nlane/analysis/rarexsec-analysis/config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = false,
            .variable_options = {}
        });

        // BDT configuration
        std::string bdt_model_output_path = "dataset/weights/TMVAClassification_BDT.weights.xml";
        std::string bdt_method_name = "BDT";
        std::string bdt_score_column_name = "bdt_score";

        // Signal and background definitions
        std::string signal_cut = "(mcf_strangeness > 0)";
        std::string background_cut = "!(mcf_strangeness > 0)";

        // Quality preselection query
        std::string quality_presel_query = AnalysisFramework::Selection::getPreselectionCategories().at("QUALITY").query.Data();

        // BDT feature columns
        std::vector<std::string> feature_column_names = {
            "nhits_u", 
            "nhits_v",
            "nhits_w", 
            "nclusters_u",
            "nclusters_v",
            "nclusters_w"
        };

        // Initialize BDT manager
        AnalysisFramework::BDTManager bdt_manager;

        // Histograms to accumulate signal and background across samples
        TH1D* total_sig_hist = nullptr;
        TH1D* total_bkg_hist = nullptr;

        // Process each Monte Carlo sample
        for (const auto& [sample_key, sample_info] : data_manager.getAllSamples()) {
            if (sample_info.isMonteCarlo()) {
                // Get the pre-processed dataframe
                ROOT::RDF::RNode df = sample_info.getDataFrame();

                // Apply quality preselection
                ROOT::RDF::RNode df_filtered = df.Filter(quality_presel_query);

                // Add BDT score column
                ROOT::RDF::RNode df_with_bdt = bdt_manager.addBDTScoreColumn(
                    df_filtered,
                    bdt_score_column_name,
                    bdt_model_output_path,
                    bdt_method_name,
                    feature_column_names
                );

                // Split into signal and background
                ROOT::RDF::RNode sig_df = df_with_bdt.Filter(signal_cut);
                ROOT::RDF::RNode bkg_df = df_with_bdt.Filter(background_cut);

                // Create histograms (weighted by base_event_weight)
                // Create TH1DModel for signal histogram
                ROOT::RDF::TH1DModel sig_model(
                    ("h_sig_bdt_" + sample_key).c_str(),
                    "Signal BDT Score;BDT Score;Events",
                    100,
                    -1.0,
                    1.0
                );

                // Create TH1DModel for background histogram
                ROOT::RDF::TH1DModel bkg_model(
                    ("h_bkg_bdt_" + sample_key).c_str(),
                    "Background BDT Score;BDT Score;Events",
                    100,
                    -1.0,
                    1.0
                );

                // Create histograms (weighted by base_event_weight)
                auto sig_hist_future = sig_df.Histo1D(
                    sig_model,
                    bdt_score_column_name,
                    "base_event_weight"
                );
                auto bkg_hist_future = bkg_df.Histo1D(
                    bkg_model,
                    bdt_score_column_name,
                    "base_event_weight"
                );

                // Retrieve histogram pointers (triggers computation)
                TH1D* sig_hist = sig_hist_future.GetPtr();
                TH1D* bkg_hist = bkg_hist_future.GetPtr();

                // Accumulate into total histograms
                if (total_sig_hist == nullptr) {
                    total_sig_hist = static_cast<TH1D*>(sig_hist->Clone("total_sig_hist"));
                    total_bkg_hist = static_cast<TH1D*>(bkg_hist->Clone("total_bkg_hist"));
                } else {
                    total_sig_hist->Add(sig_hist);
                    total_bkg_hist->Add(bkg_hist);
                }
            }
        }

        // Check if histograms were generated
        if (total_sig_hist && total_bkg_hist) {
            std::vector<double> signal_efficiency;
            std::vector<double> background_rejection;

            double total_signal_events = total_sig_hist->Integral(0, total_sig_hist->GetNbinsX() + 1);
            double total_background_events = total_bkg_hist->Integral(0, total_bkg_hist->GetNbinsX() + 1);

            std::cout << "Total signal events: " << total_signal_events << std::endl;
            std::cout << "Total background events: " << total_background_events << std::endl;

            if (total_signal_events > 0 && total_background_events > 0) {
                std::cout << "Calculating ROC curve points..." << std::endl;
                for (int i = total_sig_hist->GetNbinsX(); i >= 0; --i) {
                    double sig_pass = total_sig_hist->Integral(i + 1, total_sig_hist->GetNbinsX() + 1);
                    double bkg_pass = total_bkg_hist->Integral(i + 1, total_bkg_hist->GetNbinsX() + 1);
                    signal_efficiency.push_back(sig_pass / total_signal_events);
                    background_rejection.push_back(1.0 - (bkg_pass / total_background_events));
                }
                std::cout << "ROC curve points calculated." << std::endl;

                // Plot ROC curve
                TCanvas* c_roc = new TCanvas("c_roc", "ROC Curve", 800, 600);
                gStyle->SetGridStyle(3);
                c_roc->SetGrid();

                TGraph* roc_curve = new TGraph(signal_efficiency.size(), background_rejection.data(), signal_efficiency.data());
                roc_curve->SetTitle("ROC Curve;Background Rejection (1 - FPR);Signal Efficiency (TPR)");
                roc_curve->SetLineColor(kBlue);
                roc_curve->SetLineWidth(2);
                roc_curve->SetMarkerStyle(20);
                roc_curve->SetMarkerSize(0.8);

                roc_curve->GetXaxis()->SetRangeUser(0.0, 1.0);
                roc_curve->GetYaxis()->SetRangeUser(0.0, 1.0);

                roc_curve->Draw("APL");

                TLatex latex;
                latex.SetNDC();
                latex.SetTextAlign(33);
                latex.SetTextFont(62);
                latex.SetTextSize(0.04);
                latex.DrawLatex(0.9, 0.9, "MicroBooNE Simulation, Preliminary");

                c_roc->SaveAs("roc_curve.png");

                delete c_roc;
                delete roc_curve;
            } else {
                std::cerr << "Warning: Total signal or background events for ROC calculation is zero. Skipping ROC plot." << std::endl;
            }
        } else {
            std::cerr << "Warning: No Monte Carlo samples found or histograms could not be generated. Skipping ROC plotting." << std::endl;
        }

    } catch (const std::exception& e) {
        std::cerr << "An error occurred: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred." << std::endl;
        return 1;
    }

    return 0;
}