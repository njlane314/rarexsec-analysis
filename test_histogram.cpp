#include "SampleLoader.h"
#include "ConfigurationManager.h"
#include "VariableManager.h"
#include "Selection.h"
#include "Binning.h"
#include "HistogramGenerator.h"
#include "Histogram.h"
#include "EventCategories.h"

#include "TFile.h"
#include "TH1.h"
#include "THStack.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TString.h"
#include "TPad.h"
#include "ROOT/RDataFrame.hxx"
#include "TColor.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TLine.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>
#include <map>
#include <numeric>

int main() {
    try {
        // Set ROOT style options
        gStyle->SetErrorX(0.5);
        gStyle->SetOptStat(0); // Disable stat box for cleaner look

        // Configure file paths and managers
        AnalysisFramework::FilePathConfiguration file_paths;
        file_paths.sample_directory_base = "/exp/uboone/data/users/nlane/analysis/";
        AnalysisFramework::ConfigurationManager config_manager(file_paths);
        AnalysisFramework::VariableManager variable_manager;
        AnalysisFramework::SampleLoader loader(config_manager, variable_manager);
        std::string beam_key = "numi_fhc";
        std::vector<std::string> runs_to_load = {"run1"};
        bool blinded = true;
        AnalysisFramework::VariableOptions var_opts;

        std::cout << "Loading runs..." << std::endl;
        AnalysisFramework::SampleDataset sample_dataset = loader.LoadRuns(beam_key, runs_to_load, blinded, var_opts);
        std::cout << "Runs loaded. Processing " << sample_dataset.dataframes.size() << " samples." << std::endl;

        // Define binning for nu_e
        Analysis::Binning base_binning = Analysis::Binning::fromConfig(
            "nu_e", 10, 0.0, 10.0, "nu_e", "Neutrino Energy [GeV]"
        );
        base_binning.setSelection("NUMU_CC", "NUMU");
        std::cout << "Base Binning selection query set to: \"" << base_binning.selection_query.Data() << "\"" << std::endl;
        std::cout << "Base Binning selection TeX set to: \"" << base_binning.selection_tex.Data() << "\"" << std::endl;

        AnalysisFramework::EventCategory event_categories_manager;

        // Create output ROOT file
        TFile* output_file = new TFile("output_histograms_test.root", "RECREATE");
        if (!output_file || output_file->IsZombie()) {
            throw std::runtime_error("Failed to create output ROOT file.");
        }

        // Process each sample
        for (auto const& [sample_key, df_vector] : sample_dataset.dataframes) {
            std::cout << "Processing sample: " << sample_key << " with " << df_vector.size() << " RNode(s)." << std::endl;
            int df_index = 0;
            for (ROOT::RDF::RNode& df_node : const_cast<std::vector<ROOT::RDF::RNode>&>(df_vector)) {
                std::cout << " Processing RNode index: " << df_index << std::endl;

                // Verify nu_e exists in the dataframe
                if (!df_node.HasColumn("nu_e")) {
                    std::cerr << "Warning: 'nu_e' not found in sample " << sample_key << ", skipping.\n";
                    continue;
                }

                // Set up histogram stack
                TString stack_name = TString::Format("stack_%s_df%d", sample_key.c_str(), df_index);
                TString stack_title = TString::Format(";%s;Events", base_binning.variable_tex.Data());
                THStack* hist_stack = new THStack(stack_name, stack_title);

                // Configure legend
                TLegend* legend = new TLegend(0.65, 0.65, 0.9, 0.9); // Top-right corner
                legend->SetFillStyle(0);
                legend->SetBorderSize(0);
                legend->SetTextSize(0.035);
                legend->SetNColumns(1);

                std::vector<TH1D*> category_hists_for_cleanup;
                Analysis::Histogram total_mc_hist_obj;
                bool first_mc_hist = true;

                // Generate histograms for each category
                for (int category_id : event_categories_manager.GetAllCategoryIds()) {
                    if (category_id == 0 || category_id == 1) {
                        continue; // Skip data categories
                    }

                    AnalysisFramework::CategoryDisplayInfo cat_info = event_categories_manager.GetCategoryInfo(category_id);
                    TString category_filter = TString::Format("event_category == %d", category_id);
                    Analysis::HistogramGenerator hist_gen(df_node, base_binning, "exposure_event_weight");
                    Analysis::Histogram generated_hist_obj = hist_gen.generate(category_filter);
                    TString hist_obj_name = TString::Format("%s_cat%d", base_binning.label.Data(), category_id);
                    generated_hist_obj.SetName(hist_obj_name.Data());
                    generated_hist_obj.plot_hatch_idx = 1001;

                    std::cout << " Generated histogram '" << generated_hist_obj.GetName()
                              << "' for category '" << cat_info.short_label.c_str()
                              << "' with " << generated_hist_obj.sum() << " entries." << std::endl;

                    if (first_mc_hist) {
                        total_mc_hist_obj = generated_hist_obj;
                        total_mc_hist_obj.SetName(TString::Format("%s_total_mc", base_binning.label.Data()));
                        first_mc_hist = false;
                    } else {
                        total_mc_hist_obj = total_mc_hist_obj + generated_hist_obj;
                    }

                    TString individual_hist_name_in_file = TString::Format("hist_%s_df%d_cat%d_%s",
                        sample_key.c_str(), df_index, category_id, cat_info.short_label.c_str());
                    individual_hist_name_in_file.ReplaceAll(" ", "_").ReplaceAll("(", "").ReplaceAll(")", "")
                        .ReplaceAll("$", "").ReplaceAll("\\", "").ReplaceAll("{", "").ReplaceAll("}", "")
                        .ReplaceAll("^", "").ReplaceAll("#", "").ReplaceAll("%", "pct").ReplaceAll("+", "plus");

                    TH1D* category_th1d = generated_hist_obj.getRootHistCopy(individual_hist_name_in_file);
                    if (category_th1d) {
                        category_th1d->SetFillColor(cat_info.color);
                        category_th1d->SetLineColor(kBlack);
                        category_th1d->SetLineWidth(1);
                        category_th1d->SetFillStyle(cat_info.fill_style);

                        if (category_th1d->GetSumOfWeights() > 0) {
                            hist_stack->Add(category_th1d);
                            legend->AddEntry(category_th1d, cat_info.short_label.c_str(), "f");
                        }
                        output_file->cd();
                        category_th1d->Write(individual_hist_name_in_file, TObject::kOverwrite);
                        category_hists_for_cleanup.push_back(category_th1d);
                    }
                }

                // Set up canvas
                TString canvas_name = TString::Format("canvas_stacked_%s_df%d", sample_key.c_str(), df_index);
                TCanvas* main_canvas = new TCanvas(canvas_name, "Stacked Plot", 800, 800);
                TPad* plot_pad = new TPad("plot_pad", "Plot Pad", 0.0, 0.0, 1.0, 1.0);
                plot_pad->SetBottomMargin(0.12);
                plot_pad->SetTopMargin(0.05);
                plot_pad->SetLeftMargin(0.15);
                plot_pad->Draw();
                plot_pad->cd();

                // Handle total MC histogram with errors
                TH1D* total_th1d_for_errors = nullptr;
                if (hist_stack->GetNhists() > 0 && !first_mc_hist) {
                    total_th1d_for_errors = total_mc_hist_obj.getRootHistCopy("total_mc_errors_hist");
                    total_th1d_for_errors->SetFillColorAlpha(kGray + 2, 0.5);
                    total_th1d_for_errors->SetFillStyle(3004);
                    total_th1d_for_errors->SetMarkerSize(0);
                    legend->AddEntry(total_th1d_for_errors, "Total MC Unc.", "f");
                }

                // Draw the stack
                if (hist_stack->GetNhists() > 0) {
                    hist_stack->Draw("HIST");
                    hist_stack->GetXaxis()->SetTitle(base_binning.variable_tex.Data());
                    hist_stack->GetXaxis()->SetTitleSize(0.045);
                    hist_stack->GetXaxis()->SetLabelSize(0.04);
                    hist_stack->GetYaxis()->SetTitle("Events");
                    hist_stack->GetYaxis()->SetTitleSize(0.045);
                    hist_stack->GetYaxis()->SetLabelSize(0.04);
                    hist_stack->GetYaxis()->SetTitleOffset(1.2);

                    double y_max = hist_stack->GetMaximum();
                    if (total_th1d_for_errors) {
                        for (int i = 1; i <= total_th1d_for_errors->GetNbinsX(); ++i) {
                            double val = total_th1d_for_errors->GetBinContent(i) + total_th1d_for_errors->GetBinError(i);
                            if (val > y_max) y_max = val;
                        }
                        total_th1d_for_errors->Draw("E2 SAME");
                    }
                    hist_stack->SetMaximum(y_max * 1.2);
                    hist_stack->Draw("HIST SAME");
                } else {
                    TH1D* frame = new TH1D("empty_frame", stack_title,
                                           base_binning.nBins(), base_binning.bin_edges.data());
                    frame->SetDirectory(nullptr);
                    frame->GetXaxis()->SetTitle(base_binning.variable_tex.Data());
                    frame->GetYaxis()->SetTitle("Events");
                    frame->GetYaxis()->SetRangeUser(0., 1.);
                    frame->Draw();
                    delete frame;
                }

                legend->Draw();

                // Save the plot
                main_canvas->Update();
                TString pdf_file_name = TString::Format("stacked_hist_%s_df%d.pdf", sample_key.c_str(), df_index);
                main_canvas->SaveAs(pdf_file_name);
                std::cout << " Saved stacked histogram to " << pdf_file_name.Data() << std::endl;

                // Write to output file
                output_file->cd();
                if (hist_stack->GetNhists() > 0) {
                    hist_stack->Write(stack_name, TObject::kOverwrite);
                    if (total_th1d_for_errors) {
                        total_th1d_for_errors->Write(TString::Format("total_mc_errors_%s_df%d", sample_key.c_str(), df_index), TObject::kOverwrite);
                    }
                }

                // Clean up
                delete main_canvas;
                delete legend;
                if (total_th1d_for_errors) delete total_th1d_for_errors;
                for (TH1D* h : category_hists_for_cleanup) {
                    delete h;
                }
                delete hist_stack;
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
    std::cout << "Program finished successfully." << std::endl;
    return 0;
}