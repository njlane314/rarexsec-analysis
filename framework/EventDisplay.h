#ifndef EVENTDISPLAY_H
#define EVENTDISPLAY_H

#include "DataManager.h"
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <TH2F.h>
#include <TCanvas.h>
#include <TRandom3.h>
#include <ROOT/RDataFrame.hxx>
#include <algorithm>
#include <numeric>
#include <TLatex.h>

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

        struct EventInfo {
            std::string sample_key;
            int run;
            int sub;
            int evt;
            double weight;
        };

        std::vector<EventInfo> all_passing_events;
        std::vector<double> weights;

        for (const auto& [sample_key, sample_info] : data_manager_.getAllSamples()) {
            ROOT::RDF::RNode df = sample_info.getDataFrame();
            auto filtered_df = df.Filter(query);
            ULong64_t N = filtered_df.Count().GetValue();
            if (N == 0) continue;

            auto runs = filtered_df.Take<int>("run").GetValue();
            auto subs = filtered_df.Take<int>("sub").GetValue();
            auto evts = filtered_df.Take<int>("evt").GetValue();
            auto wts = filtered_df.Take<double>("event_weight_cv").GetValue();

            for (size_t i = 0; i < N; ++i) {
                all_passing_events.push_back({sample_key, runs[i], subs[i], evts[i], wts[i]});
                weights.push_back(wts[i]);
            }
        }

        if (all_passing_events.empty()) {
            std::cout << "No events found in the analysis space.\n";
            return;
        }

        int K = num_events;
        if (K > static_cast<int>(all_passing_events.size())) K = all_passing_events.size();

        std::vector<double> cum_weights(weights.size());
        std::partial_sum(weights.begin(), weights.end(), cum_weights.begin());
        double total_weight = cum_weights.back();

        std::vector<size_t> selected_indices;
        for (int i = 0; i < K; ++i) {
            double r = rand_gen_.Uniform(0, total_weight);
            auto it = std::lower_bound(cum_weights.begin(), cum_weights.end(), r);
            size_t idx = std::distance(cum_weights.begin(), it);
            selected_indices.push_back(idx);
        }
        for (size_t idx : selected_indices) {
            const auto& event = all_passing_events[idx];
            std::string sample_key = event.sample_key;
            int run = event.run;
            int sub = event.sub;
            int evt = event.evt;

            auto it = data_manager_.getAllSamples().find(sample_key);
            if (it == data_manager_.getAllSamples().end()) continue;
            ROOT::RDF::RNode df = it->second.getDataFrame();
            TString filter_str = TString::Format("run == %d && sub == %d && evt == %d", run, sub, evt);
            auto single_event_df = df.Filter(filter_str.Data()).Range(0,1);

            auto u_data = single_event_df.Take<std::vector<float>>("detector_image_u").GetValue().at(0);
            auto v_data = single_event_df.Take<std::vector<float>>("detector_image_v").GetValue().at(0);
            auto w_data = single_event_df.Take<std::vector<float>>("detector_image_w").GetValue().at(0);

            std::vector<std::string> planes = {"U", "V", "W"};
            for (const std::string& plane : planes) {
                const std::vector<float>& plane_data = (plane.compare("U") == 0 ? u_data : 
                                                    (plane.compare("V") == 0 ? v_data : w_data));
                TCanvas* c = new TCanvas(("c_" + plane + "_" + std::to_string(run) + "_" + 
                                        std::to_string(sub) + "_" + std::to_string(evt)).c_str(), "", 800, 800);
                c->SetLogz();
                TH2F* hist = PlotSinglePlaneHistogram(run, sub, evt, "raw", "h_raw", plane_data, plane);
                hist->Draw("COL");

                TLatex* latex = new TLatex();
                latex->SetNDC();
                latex->SetTextSize(0.03);
                latex->DrawLatex(0.1, 0.95, Form("Run %d, Subrun %d, Event %d, Plane %s", 
                                                run, sub, evt, plane.c_str()));
                
                std::string file_name = "event_display_" + plane + "_" + 
                                          std::to_string(run) + "_" + 
                                          std::to_string(sub) + "_" + 
                                          std::to_string(evt) + "_" + plane + ".png";
                c->Print(file_name.c_str());
                delete latex;
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
                                   const std::string& plane_name) {
        if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
            throw std::runtime_error("Image size mismatch");
        }

        //std::string hist_name = hist_name_prefix + "_" + plane_name + "_" +
                                std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt);
        std::string title = "Plane " + plane_name + " raw" +
                            " (Run " + std::to_string(run) +
                            ", Subrun " + std::to_string(sub) +
                            ", Event " + std::to_string(evt) + ")";

        TH2F* hist = new TH2F("", title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
        float threshold = 1.0;
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
};

} 

#endif // EVENTDISPLAY_H