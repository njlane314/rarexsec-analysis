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
#include <TROOT.h>

namespace AnalysisFramework {

class EventDisplay {
public:
    EventDisplay(const DataManager& data_manager, int img_size, const std::string& output_dir)
        : data_manager_(data_manager), img_size_(img_size), output_dir_(output_dir), rand_gen_(0) {
        gROOT->SetBatch(kTRUE);
    }

    void visualiseEventViews(const std::tuple<int, int, int>& event_id,
                             const std::string& sample_key) {
        auto [run, sub, evt] = event_id;
        std::string event_filter = "run == " + std::to_string(run) + " && sub == " + std::to_string(sub) + " && evt == " + std::to_string(evt);

        ROOT::RDF::RNode df = data_manager_.getSample(sample_key).getDataFrame().Filter(event_filter);

        auto u_detector = df.Take<std::vector<float>>("event_detector_image_u").GetValue().at(0);
        auto v_detector = df.Take<std::vector<float>>("event_detector_image_v").GetValue().at(0);
        auto w_detector = df.Take<std::vector<float>>("event_detector_image_w").GetValue().at(0);

        auto u_semantic = df.Take<std::vector<int>>("semantic_image_u").GetValue().at(0);
        auto v_semantic = df.Take<std::vector<int>>("semantic_image_v").GetValue().at(0);
        auto w_semantic = df.Take<std::vector<int>>("semantic_image_w").GetValue().at(0);

        std::vector<std::string> planes = {"U", "V", "W"};

        for (const std::string& plane : planes) {
            const std::vector<float>& detector_data = (plane == "U" ? u_detector : (plane == "V" ? v_detector : w_detector));
            const std::vector<int>& semantic_data = (plane == "U" ? u_semantic : (plane == "V" ? v_semantic : w_semantic));

            std::string title_detector = "Detector Plane " + plane + " (Run " + std::to_string(run) + ", Subrun " + std::to_string(sub) + ", Event " + std::to_string(evt) + ")";
            TH2F* hist_detector = plotDetectorView(run, sub, evt, "detector", "h_detector", detector_data, plane, title_detector);

            TCanvas* c_detector = new TCanvas(("c_detector_" + plane + "_" + std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt)).c_str(), "", 1200, 1200);
            gStyle->SetTitleY(0.96);
            c_detector->SetLogz();
            hist_detector->Draw("COL");
            std::string file_name_detector = output_dir_ + "/event_display_" + plane + "_" + std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt) + ".png";
            c_detector->Print(file_name_detector.c_str());
            delete c_detector;
            delete hist_detector;

            std::string title_semantic = "Semantic Plane " + plane + " (Run " + std::to_string(run) + ", Subrun " + std::to_string(sub) + ", Event " + std::to_string(evt) + ")";
            TH2F* hist_semantic = plotSemanticView(run, sub, evt, "semantic", "h_semantic", semantic_data, plane, title_semantic);

            const double PLOT_LEGEND_SPLIT = 0.85;
            TCanvas* c_semantic = new TCanvas(("c_semantic_" + plane + "_" + std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt)).c_str(), "", 1200, 800);
            
            TPad* main_pad = new TPad("main_pad", "main_pad", 0.0, 0.0, 1.0, PLOT_LEGEND_SPLIT);
            main_pad->SetTopMargin(0.01);
            main_pad->SetBottomMargin(0.12);
            main_pad->SetLeftMargin(0.12);
            main_pad->SetRightMargin(0.05);
            main_pad->Draw();
            main_pad->cd();
            hist_semantic->Draw("COL");

            c_semantic->cd();
            TPad* legend_pad = new TPad("legend_pad", "legend_pad", 0.0, PLOT_LEGEND_SPLIT, 1.0, 1.0);
            legend_pad->SetTopMargin(0.05);
            legend_pad->SetBottomMargin(0.01);
            legend_pad->Draw();
            legend_pad->cd();

            std::set<int> unique_labels(semantic_data.begin(), semantic_data.end());
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
                if (label_idx > 0 && label_idx < static_cast<int>(truth_primary_label_names.size())) {
                    TH1F* h_leg = new TH1F("", "", 1, 0, 1);
                    h_leg->SetFillColor(label_colors[label_idx]);
                    h_leg->SetLineColor(kBlack);
                    h_leg->SetLineWidth(1.5);
                    legend->AddEntry(h_leg, truth_primary_label_names[label_idx].c_str(), "f");
                }
            }
            legend->Draw();

            std::string file_name_semantic = output_dir_ + "/semantic_event_display_" + plane + "_" + std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt) + ".png";
            c_semantic->Print(file_name_semantic.c_str());
            delete c_semantic;
        }
    }

private:
    const DataManager& data_manager_;
    int img_size_;
    std::string output_dir_;
    TRandom3 rand_gen_;

    TH2F* plotDetectorView(int run, int sub, int evt,
                           const std::string& plot_type_name,
                           const std::string& hist_name_prefix,
                           const std::vector<float>& plane_data,
                           const std::string& plane_name,
                           const std::string& title) {
        TH2F* hist = new TH2F("", title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
        float threshold = 4.0;
        float min_display_value = 1.0;

        for (int r = 0; r < img_size_; ++r) {
            int row_offset = r * img_size_;
            for (int c = 0; c < img_size_; ++c) {
                float value = plane_data[row_offset + c];
                hist->SetBinContent(c + 1, r + 1, value > threshold ? value : min_display_value);
            }
        }

        hist->SetMinimum(1.0);
        hist->SetMaximum(1000.0);
        hist->GetXaxis()->SetTitle("Local Wire Coordinate");
        hist->GetYaxis()->SetTitle("Local Drift Time");
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

    TH2F* plotSemanticView(int run, int sub, int evt,
                           const std::string& plot_type_name,
                           const std::string& hist_name_prefix,
                           const std::vector<int>& plane_data,
                           const std::string& plane_name,
                           const std::string& title) {
        TH2F* hist = new TH2F((hist_name_prefix + "_" + plane_name).c_str(), title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);

        enum TruthPrimaryLabel { Empty = 0, Cosmic, Muon, Proton, Pion, ChargedKaon, NeutralKaon, Lambda, ChargedSigma, Other };
        static const std::array<int, 10> label_colors = { kWhite, kGray + 1, kRed, kBlue, kGreen + 1, kMagenta, kCyan, kOrange, kViolet, kTeal };
        
        Int_t palette[10];
        for(int i = 0; i < 10; ++i) palette[i] = label_colors[i];
        gStyle->SetPalette(10, palette);

        for (int r = 0; r < img_size_; ++r) {
            int row_offset = r * img_size_;
            for (int c = 0; c < img_size_; ++c) {
                int value = plane_data[row_offset + c];
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

#endif