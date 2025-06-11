#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <memory>

#include "AnalysisFramework.h"
#include "TCanvas.h"
#include "TGraph.h"
#include "TAxis.h"
#include "TLegend.h"
#include "TFile.h"

void calculate_efficiency_purity(
    AnalysisFramework::DataManager& data_manager,
    const std::string& selection_key,
    long& n_signal_passed,
    long& n_total_passed)
{
    long signal_count = 0;
    long total_count = 0;

    std::string full_selection_query = AnalysisFramework::Selection::getSelectionQuery(selection_key.c_str(), "QUALITY").Data();
    std::string signal_query = "analysis_channel == 10 || analysis_channel == 11";

    for (const auto& sample_pair : data_manager.getAllSamples()) {
        if (sample_pair.second.isMonteCarlo()) {
            auto df = sample_pair.second.getDataFrame();
            auto filtered_df = df.Filter(full_selection_query);

            total_count += *filtered_df.Count();

            auto signal_df = filtered_df.Filter(signal_query);
            signal_count += *signal_df.Count();
        }
    }
    n_signal_passed = signal_count;
    n_total_passed = total_count;
}


int main() {
    try {
        ROOT::EnableImplicitMT();

        AnalysisFramework::DataManager data_manager({
            .config_file = "../config.json",
            .beam_key = "numi_fhc",
            .runs_to_load = {"run1"},
            .blinded = true,
            .variable_options = {
                .load_reco_event_info = true, .load_reco_track_info = true,
                .load_truth_event_info = true, .load_weights_and_systematics = true
            }
        });

        std::vector<std::pair<std::string, std::string>> selection_stages = {
            {"QUALITY", "Quality Presel."},
            {"NUMU_CC_LOOSE", "Loose NuMu CC"},
            {"NUMU_CC_TIGHT", "Tight NuMu CC"},
            {"TRACK_SCORE", "Track Score"}
        };

        long n_signal_total = 0;
        for (const auto& sample_pair : data_manager.getAllSamples()) {
            if (sample_pair.second.isMonteCarlo()) {
                 auto df = sample_pair.second.getDataFrame();
                 n_signal_total += *df.Filter("analysis_channel == 10 || analysis_channel == 11").Count();
            }
        }


        std::vector<double> efficiencies;
        std::vector<double> purities;
        std::vector<double> stage_indices;

        for (size_t i = 0; i < selection_stages.size(); ++i) {
            long n_signal_passed = 0;
            long n_total_passed = 0;

            calculate_efficiency_purity(data_manager, selection_stages[i].first, n_signal_passed, n_total_passed);

            double efficiency = (n_signal_total > 0) ? static_cast<double>(n_signal_passed) / n_signal_total : 0.0;
            double purity = (n_total_passed > 0) ? static_cast<double>(n_signal_passed) / n_total_passed : 0.0;

            efficiencies.push_back(efficiency);
            purities.push_back(purity);
            stage_indices.push_back(i + 1);

            std::cout << "Stage: " << selection_stages[i].second
                      << ", Efficiency: " << efficiency
                      << ", Purity: " << purity << std::endl;
        }

        TCanvas* c1 = new TCanvas("c1", "Selection Efficiency and Purity", 800, 600);
        c1->SetGrid();

        TGraph* gr_eff = new TGraph(selection_stages.size(), stage_indices.data(), efficiencies.data());
        gr_eff->SetTitle("Selection Efficiency and Purity");
        gr_eff->SetMarkerStyle(20);
        gr_eff->SetMarkerColor(kBlue);
        gr_eff->SetLineColor(kBlue);
        gr_eff->GetXaxis()->SetTitle("Selection Stage");
        gr_eff->GetYaxis()->SetTitle("Fraction");
        gr_eff->GetYaxis()->SetRangeUser(0.0, 1.1);

        for (size_t i = 0; i < selection_stages.size(); ++i) {
            gr_eff->GetXaxis()->SetBinLabel(gr_eff->GetXaxis()->FindBin(i + 1), selection_stages[i].second.c_str());
        }

        gr_eff->Draw("APL");

        TGraph* gr_pur = new TGraph(selection_stages.size(), stage_indices.data(), purities.data());
        gr_pur->SetMarkerStyle(21);
        gr_pur->SetMarkerColor(kRed);
        gr_pur->SetLineColor(kRed);
        gr_pur->Draw("PL SAME");

        TLegend* legend = new TLegend(0.7, 0.7, 0.9, 0.9);
        legend->AddEntry(gr_eff, "Efficiency", "pl");
        legend->AddEntry(gr_pur, "Purity", "pl");
        legend->Draw();

        c1->SaveAs("plots/selection_efficiency_purity.png");

        delete c1;
        delete gr_eff;
        delete gr_pur;
        delete legend;

    } catch (const std::exception& e) {
        std::cerr << "Exception caught: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}