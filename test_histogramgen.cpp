#include "DatasetLoader.h"
#include "ConfigurationManager.h"
#include "VariableManager.h"
#include "Selection.h"
#include "Binning.h"
#include "HistogramGenerator.h"
#include "Histogram.h"

#include "TFile.h"
#include "TH1.h"
#include "TCanvas.h"
#include "TString.h"
#include "ROOT/RDataFrame.hxx"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

int main() {
    try {
        AnalysisFramework::FilePathConfiguration file_paths;
        file_paths.sample_directory_base = "/exp/uboone/data/users/nlane/analysis/";
        AnalysisFramework::ConfigurationManager config_manager(file_paths);
        AnalysisFramework::VariableManager variable_manager;
        AnalysisFramework::DatasetLoader loader(config_manager, variable_manager);
        
        std::string beam_key = "numi_fhc";
        std::vector<std::string> runs_to_load = {"run1"};
        bool blinded = true;
        AnalysisFramework::VariableOptions var_opts;

        std::cout << "Loading runs..." << std::endl;
        AnalysisFramework::CampaignDataset campaign = loader.LoadRuns(beam_key, runs_to_load, blinded, var_opts);
        std::cout << "Runs loaded. Processing " << campaign.dataframes.size() << " samples." << std::endl;

        Analysis::Binning binning = Analysis::Binning::fromConfig(
            "nu_e", 10, 0.0, 10.0, "nu_e", "Neutrino Energy [GeV]"
        );
        
        // Corrected selection: Using "NUMU_CC" as the selection key and "NUMU" as the preselection key.
        // Adjust these keys if your intended selection is different.
        // The original call was: binning.setSelection("numu_cc_selected", "$\\nu_{\\mu}$ CC Selected");
        // which would not find matching keys in your Selection.h and result in an empty selection_query.
        binning.setSelection("NUMU_CC", "NUMU"); 
        std::cout << "Binning selection query set to: \"" << binning.selection_query.Data() << "\"" << std::endl;
        std::cout << "Binning selection TeX set to: \"" << binning.selection_tex.Data() << "\"" << std::endl;


        TFile* output_file = new TFile("output_histograms_test.root", "RECREATE");
        if (!output_file || output_file->IsZombie()) {
            throw std::runtime_error("Failed to create output ROOT file.");
        }

        for (auto const& [sample_key, df_vector] : campaign.dataframes) {
            std::cout << "Processing sample: " << sample_key << " with " << df_vector.size() << " RNode(s)." << std::endl;
            int df_index = 0;
            for (ROOT::RDF::RNode& df_node : const_cast<std::vector<ROOT::RDF::RNode>&>(df_vector)) {
                std::cout << "  Processing RNode index: " << df_index << std::endl;
                Analysis::HistogramGenerator hist_gen(df_node, binning, "exposure_event_weight");
                Analysis::Histogram generated_hist = hist_gen.generate();
                
                std::cout << "    Generated histogram '" << generated_hist.GetName() << "' with " << generated_hist.sum() << " entries." << std::endl;

                TString copiedHistName = TString::Format("hist_%s_df%d", sample_key.c_str(), df_index);
                TH1D* root_hist_to_draw_and_save = generated_hist.getRootHistCopy(copiedHistName);

                if (root_hist_to_draw_and_save) {
                    TString canvasName = TString::Format("canvas_%s_df%d", sample_key.c_str(), df_index);
                    TString canvasTitle = TString::Format("Histogram: %s (DF %d) - %s", sample_key.c_str(), df_index, generated_hist.GetTitle());
                    TCanvas* canvas = new TCanvas(canvasName, canvasTitle, 800, 600);
                    
                    root_hist_to_draw_and_save->SetTitle(canvasTitle);
                    root_hist_to_draw_and_save->Draw("E1 HIST");
                    canvas->Update();
                    
                    TString pngFileName = TString::Format("hist_%s_df%d.png", sample_key.c_str(), df_index);
                    canvas->SaveAs(pngFileName);
                    std::cout << "    Saved histogram to " << pngFileName.Data() << std::endl;
                    
                    output_file->cd();
                    root_hist_to_draw_and_save->Write();
                    std::cout << "    Written histogram " << copiedHistName.Data() << " to ROOT file." << std::endl;

                    delete canvas;
                    delete root_hist_to_draw_and_save;

                } else {
                    std::cerr << "Warning: Could not get ROOT histogram copy for " << sample_key << ", df_index " << df_index << std::endl;
                }
                df_index++;
            }
        }

        std::cout << "All samples processed. Closing output file." << std::endl;
        output_file->Close();
        delete output_file;

    } catch (const std::exception& e) {
        std::cerr << "Error in main: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "An unknown error occurred in main." << std::endl;
        return 2;
    }
    std::cout << "Test program finished successfully." << std::endl;
    return 0;
}
