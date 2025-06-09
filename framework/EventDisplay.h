#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H

#include "DataManager.h"
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <set>
#include <TH2F.h>
#include <TCanvas.h>
#include <TRandom3.h>
#include <ROOT/RDataFrame.hxx>
#include <algorithm>
#include <numeric>
#include <TLatex.h>
#include <TStyle.h>
#include <TPaletteAxis.h>
#include <array>
#include <TLegend.h>
#include <TPad.h> 

namespace AnalysisFramework {

class EventDisplay {
public:
    EventDisplay(const DataManager& data_manager, int img_size, const std::string& output_dir)
        : data_manager_(data_manager), img_size_(img_size), output_dir_(output_dir), rand_gen_(0) {}

    void VisualiseEventsInRegion(const std::string& selection_key,
                                 const std::string& preselection_key,
                                 const std::string& additional_selection = "",
                                 int num_events = 1,
                                 const std::string& output_file = "event_display.pdf") {
        std::string query = Selection::getSelectionQuery(TString(selection_key.c_str()), TString(preselection_key.c_str())).Data();
        if (!additional_selection.empty()) {
            query += " && " + additional_selection;
        }

        std::vector<ROOT::RDF::RNode> filtered_dfs;
        std::vector<ULong64_t> counts;

        for (const auto& [sample_key, sample_info] : data_manager_.getAllSamples()) {
            if (!sample_info.isMonteCarlo()) continue;
            ROOT::RDF::RNode df = sample_info.getDataFrame();
            auto filtered_df = df.Filter(query);
            ULong64_t N = filtered_df.Count().GetValue();
            if (N > 0) {
                filtered_dfs.push_back(filtered_df);
                counts.push_back(N);
            }
        }

        ULong64_t total_events = std::accumulate(counts.begin(), counts.end(), 0ULL);

        if (total_events == 0) {
            std::cout << "No events found in the analysis space.\n";
            return;
        }

        int K = std::min((ULong64_t)num_events, total_events);

        for (int i = 0; i < K; ++i) {
            ULong64_t random_event_index = rand_gen_.Integer(total_events);
            ULong64_t cumulative_count = 0;
            size_t sample_idx = 0;
            for(size_t j=0; j<counts.size(); ++j){
                cumulative_count += counts[j];
                if(random_event_index < cumulative_count){
                    sample_idx = j;
                    break;
                }
            }
            ULong64_t index_in_sample = random_event_index - (cumulative_count - counts[sample_idx]);

            auto single_event_df = filtered_dfs[sample_idx].Range(index_in_sample, index_in_sample + 1);

            int run = single_event_df.Take<int>("run").GetValue()[0];
            int sub = single_event_df.Take<int>("sub").GetValue()[0];
            int evt = single_event_df.Take<int>("evt").GetValue()[0];

            auto u_data = single_event_df.Take<std::vector<float>>("raw_image_u").GetValue().at(0);
            auto v_data = single_event_df.Take<std::vector<float>>("raw_image_v").GetValue().at(0);
            auto w_data = single_event_df.Take<std::vector<float>>("raw_image_w").GetValue().at(0);

            std::vector<std::string> planes = {"U", "V", "W"};
            for (const std::string& plane : planes) {
                const std::vector<float>& plane_data = (plane.compare("U") == 0 ? u_data : 
                                                    (plane.compare("V") == 0 ? v_data : w_data));
                TCanvas* c = new TCanvas(("c_" + plane + "_" + std::to_string(run) + "_" + 
                                        std::to_string(sub) + "_" + std::to_string(evt)).c_str(), "", 1200, 1000);
                gStyle->SetTitleY(0.96); 
                c->SetLogz();
                
                std::string title = "Plane " + plane +
                            " (Run " + std::to_string(run) +
                            ", Subrun " + std::to_string(sub) + ", Event " + std::to_string(evt) + ")";
                TH2F* hist = PlotSinglePlaneHistogram(run, sub, evt, "raw", "h_raw", plane_data, plane, title);
                hist->Draw("COL");
                
                std::string file_name = "./event_display_" + plane + "_" + 
                                          std::to_string(run) + "_" + 
                                          std::to_string(sub) + "_" + 
                                          std::to_string(evt) + ".png";
                c->Print(file_name.c_str());
                delete c;
            }
        }
    }

    void VisualiseTrueEventsInRegion(const std::string& selection_key,
                                 const std::string& preselection_key,
                                 const std::string& additional_selection = "",
                                 int num_events = 1) {
        std::string query = Selection::getSelectionQuery(TString(selection_key.c_str()), TString(preselection_key.c_str())).Data();
        if (!additional_selection.empty()) {
            query += " && " + additional_selection;
        }

        std::vector<ROOT::RDF::RNode> filtered_dfs;
        std::vector<ULong64_t> counts;

        for (const auto& [sample_key, sample_info] : data_manager_.getAllSamples()) {
             if (!sample_info.isMonteCarlo()) continue;
            ROOT::RDF::RNode df = sample_info.getDataFrame();
            auto filtered_df = df.Filter(query);
            ULong64_t N = filtered_df.Count().GetValue();
            if (N > 0) {
                filtered_dfs.push_back(filtered_df);
                counts.push_back(N);
            }
        }
        
        ULong64_t total_events = std::accumulate(counts.begin(), counts.end(), 0ULL);

        if (total_events == 0) {
            std::cout << "No events found in the analysis space.\n";
            return;
        }
        
        int K = std::min((ULong64_t)num_events, total_events);

        for (int i = 0; i < K; ++i) {
            ULong64_t random_event_index = rand_gen_.Integer(total_events);
            ULong64_t cumulative_count = 0;
            size_t sample_idx = 0;
             for(size_t j=0; j<counts.size(); ++j){
                cumulative_count += counts[j];
                if(random_event_index < cumulative_count){
                    sample_idx = j;
                    break;
                }
            }
            ULong64_t index_in_sample = random_event_index - (cumulative_count - counts[sample_idx]);

            auto single_event_df = filtered_dfs[sample_idx].Range(index_in_sample, index_in_sample + 1);
            
            int run = single_event_df.Take<int>("run").GetValue()[0];
            int sub = single_event_df.Take<int>("sub").GetValue()[0];
            int evt = single_event_df.Take<int>("evt").GetValue()[0];

            auto u_data = single_event_df.Take<std::vector<int>>("true_image_u").GetValue().at(0);
            auto v_data = single_event_df.Take<std::vector<int>>("true_image_v").GetValue().at(0);
            auto w_data = single_event_df.Take<std::vector<int>>("true_image_w").GetValue().at(0);

            std::vector<std::string> planes = {"U", "V", "W"};
            for (const std::string& plane : planes) {
                const std::vector<int>& plane_data = (plane.compare("U") == 0 ? u_data : 
                                                    (plane.compare("V") == 0 ? v_data : w_data));
                
                const double PLOT_LEGEND_SPLIT = 0.85;
                TCanvas* c = new TCanvas(("c_true_" + plane + "_" + std::to_string(run) + "_" + 
                                        std::to_string(sub) + "_" + std::to_string(evt)).c_str(), "", 1200, 800);
                
                TPad* main_pad = new TPad("main_pad", "main_pad", 0.0, 0.0, 1.0, PLOT_LEGEND_SPLIT);
                main_pad->SetTopMargin(0.01);
                main_pad->SetBottomMargin(0.12);
                main_pad->SetLeftMargin(0.12);
                main_pad->SetRightMargin(0.05);
                main_pad->Draw();
                main_pad->cd();

                std::string title = "True Plane " + plane +
                            " (Run " + std::to_string(run) +
                            ", Subrun " + std::to_string(sub) + ", Event " + std::to_string(evt) + ")";
                TH2F* hist = PlotSinglePlaneHistogram(run, sub, evt, "true", "h_true", plane_data, plane, title);
                hist->Draw("COL");
                
                c->cd();
                TPad* legend_pad = new TPad("legend_pad", "legend_pad", 0.0, PLOT_LEGEND_SPLIT, 1.0, 1.0);
                legend_pad->SetTopMargin(0.05);
                legend_pad->SetBottomMargin(0.01);
                legend_pad->Draw();
                legend_pad->cd();

                std::set<int> unique_labels(plane_data.begin(), plane_data.end());
                TLegend* legend = new TLegend(0.1, 0.0, 0.9, 1.0);
                legend->SetBorderSize(0);
                legend->SetFillStyle(0);
                legend->SetTextFont(42);

                int n_cols = (unique_labels.size() > 4) ? 3 : 2;
                legend->SetNColumns(n_cols);

                enum TruthPrimaryLabel { Empty = 0, Cosmic, Muon, Proton, Pion, ChargedKaon, NeutralKaon, Lambda, ChargedSigma, Other };
                static const std::array<std::string, 10> truth_primary_label_names = { "Empty", "Cosmic", "Muon", "Proton", "Pion", "ChargedKaon", "NeutralKaon", "Lambda", "ChargedSigma", "Other" };
                static const std::array<int, 10> label_colors = { kWhite, kGray + 1, kRed, kBlue, kGreen + 1, kMagenta, kCyan, kOrange, kViolet, kTeal };

                for (int label_idx : unique_labels) {
                    if (label_idx > 0 && label_idx < (int)truth_primary_label_names.size()) { // Don't add "Empty" to legend
                        TH1F* h_leg = new TH1F("", "", 1, 0, 1);
                        h_leg->SetFillColor(label_colors[label_idx]);
                        h_leg->SetLineColor(kBlack);
                        h_leg->SetLineWidth(1.5);
                        legend->AddEntry(h_leg, truth_primary_label_names[label_idx].c_str(), "f");
                    }
                }
                legend->Draw();

                std::string file_name = "./true_event_display_" + plane + "_" + 
                                          std::to_string(run) + "_" + 
                                          std::to_string(sub) + "_" + 
                                          std::to_string(evt) + ".png";
                c->Print(file_name.c_str());
                delete c;
            }
        }
    }


private:
    const DataManager& data_manager_;
    int img_size_;
    std::string output_dir_;
    std::vector<std::string> plane_names_ = {"U", "V", "W"};
    TRandom3 rand_gen_;

    TH2F* PlotSinglePlaneHistogram(int run, int sub, int evt,
                                   const std::string& plot_type_name,
                                   const std::string& hist_name_prefix,
                                   const std::vector<float>& plane_data,
                                   const std::string& plane_name,
                                   const std::string& title) {
        if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
            throw std::runtime_error("Image size mismatch");
        }
        
        TH2F* hist = new TH2F("", title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
        float threshold = 4.0;
        float min_display_value = 1.0;

        for (int r = 0; r < img_size_; ++r) {
            for (int c = 0; c < img_size_; ++c) {
                float value = plane_data[r * img_size_ + c];
                hist->SetBinContent(c + 1, r + 1, value > threshold ? value : min_display_value);
            }
        }

        hist->SetMinimum(1.0);
        hist->SetMaximum(1000.0);
        
        hist->GetXaxis()->SetTitle("Local Drift Time");
        hist->GetYaxis()->SetTitle("Local Wire Coordinate");
        hist->GetXaxis()->SetTitleOffset(1.1f);
        hist->GetYaxis()->SetTitleOffset(1.1f);
        hist->GetXaxis()->SetLabelColor(kBlack);
        hist->GetYaxis()->SetLabelColor(kBlack);
        hist->GetXaxis()->SetTitleColor(kBlack);
        hist->GetYaxis()->SetTitleColor(kBlack);
        hist->GetXaxis()->SetNdivisions(1); 
        hist->GetYaxis()->SetNdivisions(1); 
        hist->GetXaxis()->SetTickLength(0);
        hist->GetYaxis()->SetTickLength(0);
        hist->GetXaxis()->CenterTitle();
        hist->GetYaxis()->CenterTitle();
        hist->SetStats(0); 

        return hist;
    }

    TH2F* PlotSinglePlaneHistogram(int run, int sub, int evt,
                                   const std::string& plot_type_name,
                                   const std::string& hist_name_prefix,
                                   const std::vector<int>& plane_data,
                                   const std::string& plane_name,
                                   const std::string& title) {
        if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
            throw std::runtime_error("Image size mismatch for true event display");
        }
        
        TH2F* hist = new TH2F((hist_name_prefix + "_" + plane_name).c_str(), title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);

        enum TruthPrimaryLabel { Empty = 0, Cosmic, Muon, Proton, Pion, ChargedKaon, NeutralKaon, Lambda, ChargedSigma, Other };
        static const std::array<int, 10> label_colors = { kWhite, kGray + 1, kRed, kBlue, kGreen + 1, kMagenta, kCyan, kOrange, kViolet, kTeal };
        
        Int_t palette[10];
        for(int i=0; i<10; ++i) palette[i] = label_colors[i];
        gStyle->SetPalette(10, palette);

        for (int r = 0; r < img_size_; ++r) {
            for (int c = 0; c < img_size_; ++c) {
                int value = plane_data[r * img_size_ + c];
                hist->SetBinContent(c + 1, r + 1, value);
            }
        }
        
        hist->SetStats(0);
        hist->GetZaxis()->SetRangeUser(-0.5, 9.5);
        
        hist->GetXaxis()->SetTitle("Local Drift Time");
        hist->GetYaxis()->SetTitle("Local Wire Coordinate");
        hist->GetXaxis()->SetTitleOffset(1.1f);
        hist->GetYaxis()->SetTitleOffset(1.1f);
        hist->GetXaxis()->CenterTitle();
        hist->GetYaxis()->CenterTitle();
        hist->GetXaxis()->SetNdivisions(1); 
        hist->GetYaxis()->SetNdivisions(1); 
        hist->GetXaxis()->SetTickLength(0);
        hist->GetYaxis()->SetTickLength(0);

        return hist;
    }
};

} 

#endif // EVENTDISPLAY_H