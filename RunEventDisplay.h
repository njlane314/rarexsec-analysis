#ifndef RUNEVENTDISPLAY_H
#define RUNEVENTDISPLAY_H

#include "DataLoader.h"
#include <string>
#include <vector>
#include <tuple>
#include <stdexcept>
#include <TH2F.h>
#include <TCanvas.h>
#include <TPad.h> 
#include <TRandom3.h>
#include <ROOT/RDataFrame.hxx>
#include <ROOT/RResultPtr.hxx>
#include <iostream>
#include <TString.h>
#include <TColor.h>
#include <algorithm> 

namespace AnalysisFramework {

class RunEventDisplay {
public:
    RunEventDisplay(const DataLoader::DataFramesDict& dataframes_dict, int img_size, const std::string& output_dir,
                    int run = -1, int sub = -1, int evt = -1)
        : dataframes_dict_(dataframes_dict), img_size_(img_size), output_dir_(output_dir),
          specific_run_(run), specific_subrun_(sub), specific_event_(evt),
          use_specific_event_(run >= 0 && sub >= 0 && evt >= 0), rand_gen_(0) {}

    void VisualiseInput(const std::string& sample_key) {
        ProcessVisualisation(sample_key, "raw", "Input", "h_input");
    }

    void VisualiseReco(const std::string& sample_key) {
        ProcessVisualisation(sample_key, "reco", "Reco", "h_reco");
    }

    void VisualiseTruth(const std::string& sample_key) {
        ProcessVisualisation(sample_key, "true", "Truth", "h_truth");
    }

private:
    const DataLoader::DataFramesDict& dataframes_dict_;
    int img_size_;
    std::string output_dir_;
    int specific_run_;
    int specific_subrun_;
    int specific_event_;
    bool use_specific_event_;
    std::vector<std::string> plane_names_ = {"U", "V", "W"};
    TRandom3 rand_gen_;

    const int canvas_width_ = 1200;
    const int canvas_height_ = 1200;
    const int font_style_ = 132; 

    struct EventPlanesData {
        std::vector<float> u_plane_data;
        std::vector<float> v_plane_data;
        std::vector<float> w_plane_data;
    };

    std::tuple<std::vector<float>, std::vector<float>, std::vector<float>>
    GetEventPlaneData(ROOT::RDF::RNode df_for_single_event, const std::string& column_prefix) {
        std::string col_u_name = column_prefix + "_image_u";
        std::string col_v_name = column_prefix + "_image_v";
        std::string col_w_name = column_prefix + "_image_w";
        std::string defined_struct_name = "event_planes_struct_" + column_prefix;

        auto df_with_defined_struct = df_for_single_event.Define(defined_struct_name,
            [](const std::vector<float>& u, const std::vector<float>& v, const std::vector<float>& w) {
                return EventPlanesData{u, v, w};
            }, {col_u_name, col_v_name, col_w_name});

        auto event_data_results_ptr = df_with_defined_struct.template Take<EventPlanesData>(defined_struct_name);
        std::vector<EventPlanesData> event_data_results = event_data_results_ptr.GetValue();

        if (event_data_results.empty()) {
            throw std::runtime_error("Take for defined EventPlanesData struct returned an empty result vector for " + column_prefix);
        }
        
        if (event_data_results.size() != 1) {
            throw std::runtime_error("Expected Take for defined EventPlanesData struct to yield one result for " + column_prefix + 
                                     ", but got size: " + std::to_string(event_data_results.size()) +
                                     ". This can happen if the input RDataFrame to GetEventPlaneData contained more than one event.");
        }

        const auto& single_event_data = event_data_results[0];
        return {single_event_data.u_plane_data, single_event_data.v_plane_data, single_event_data.w_plane_data};
    }

    TH2F* PlotSinglePlaneHistogram(int run, int sub, int evt,
                                   const std::string& plot_type_name, 
                                   const std::string& hist_name_prefix,    
                                   const std::vector<float>& plane_data,
                                   const std::string& current_plane_name) {
        
        if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
            throw std::runtime_error("Image size mismatch for " + plot_type_name + " plane " + current_plane_name +
                                     " (Run " + std::to_string(run) + ", Sub " + std::to_string(sub) + ", Evt " + std::to_string(evt) + "). Expected " +
                                     std::to_string(img_size_ * img_size_) + ", got " + std::to_string(plane_data.size()));
        }

        std::string hist_name = hist_name_prefix + "_" + current_plane_name + "_" + std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt);
        std::string title = "Plane " + current_plane_name + " " + plot_type_name +
                            " (Run " + std::to_string(run) +
                            ",Subrun " + std::to_string(sub) + ",Event " + std::to_string(evt) + ")";

        TH2F* h_current = new TH2F(hist_name.c_str(), title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
        
        float threshold = 1.0; 
        float min_display_value = 1.0; 

        for (int r_idx = 0; r_idx < img_size_; ++r_idx) { 
            for (int c_idx = 0; c_idx < img_size_; ++c_idx) { 
                float value = plane_data[r_idx * img_size_ + c_idx]; 
                
                if (value > threshold) {
                    h_current->SetBinContent(c_idx + 1, r_idx + 1, value); 
                } else {
                    h_current->SetBinContent(c_idx + 1, r_idx + 1, min_display_value);
                }
            }
        }
        
        SetHistogramStyle(h_current);
        h_current->SetMinimum(1.0);
        h_current->SetMaximum(1000.0); 
            
        h_current->Draw("COL");
        return h_current;
    }

    void ProcessVisualisation(const std::string& sample_key,
                              const std::string& column_prefix, 
                              const std::string& plot_type_name, 
                              const std::string& hist_name_prefix) { 
        auto [run, sub, evt] = GetEventIdentifiers(sample_key);
        
        auto it = dataframes_dict_.find(sample_key);
        if (it == dataframes_dict_.end()) {
            throw std::runtime_error("Sample key not found in dataframes_dict: " + sample_key);
        }

        const auto& rnodes_vec = it->second.second; 
        bool event_found_in_any_rnode = false;

        for (const auto& rnode_const_ref : rnodes_vec) { 
            auto rnode = rnode_const_ref; 
            
            TString filter_str = TString::Format("run == %d && sub == %d && evt == %d", run, sub, evt);
            auto filtered_df_potentially_multiple = rnode.Filter(filter_str.Data());
            
            ULong64_t num_matching_events = filtered_df_potentially_multiple.Count().GetValue();

            if (num_matching_events > 0) {
                event_found_in_any_rnode = true;
                ROOT::RDF::RNode single_event_df = filtered_df_potentially_multiple;
                if (num_matching_events > 1) {
                    std::cout << "Warning: Found " << num_matching_events << " entries for "
                              << "Run " << run << ", Subrun " << sub << ", Event " << evt
                              << " in sample " << sample_key << " (column prefix: " << column_prefix << "). "
                              << "Visualizing the first one found." << std::endl;
                    single_event_df = filtered_df_potentially_multiple.Range(0, 1); 
                }

                auto [plane_u_data, plane_v_data, plane_w_data] = GetEventPlaneData(single_event_df, column_prefix);
                std::vector<const std::vector<float>*> all_planes_data = {&plane_u_data, &plane_v_data, &plane_w_data};

                for (int plane_idx = 0; plane_idx < 3; ++plane_idx) {
                    const auto& current_plane_data = *all_planes_data[plane_idx];
                    const std::string& current_plane_name = plane_names_[plane_idx];

                    std::string canvas_name = "c_" + plot_type_name + "_" + current_plane_name + "_" +
                                              std::to_string(run) + "_" + std::to_string(sub) + "_" + std::to_string(evt);
                    std::string canvas_title_text = "Plane " + current_plane_name + " " + plot_type_name +
                                               " (R:" + std::to_string(run) + ",S:" + std::to_string(sub) + ",E:" + std::to_string(evt) + ")";

                    TCanvas* c_plane = new TCanvas(canvas_name.c_str(), canvas_title_text.c_str(), canvas_width_, canvas_height_);
                    c_plane->SetFillColor(kWhite); 
                    c_plane->SetLeftMargin(0.085f);   
                    c_plane->SetRightMargin(0.085f);  
                    c_plane->SetBottomMargin(0.085f); 
                    c_plane->SetTopMargin(0.085f);    
                    c_plane->SetLogz();

                    TH2F* h_drawn = PlotSinglePlaneHistogram(run, sub, evt, plot_type_name, hist_name_prefix, current_plane_data, current_plane_name);
                    
                    std::string plot_type_name_lower = plot_type_name;
                    std::transform(plot_type_name_lower.begin(), plot_type_name_lower.end(), plot_type_name_lower.begin(), ::tolower);

                    std::string output_filename = output_dir_ + "/" + plot_type_name_lower + "_run" + std::to_string(run) +
                                                  "_sub" + std::to_string(sub) + "_evt" + std::to_string(evt) +
                                                  "_plane" + current_plane_name + ".png";
                    c_plane->SaveAs(output_filename.c_str());

                    delete c_plane; 
                    delete h_drawn; 
                }
                return; 
            }
        }
        if (!event_found_in_any_rnode) {
            std::cerr << "Event not found in sample: " << sample_key 
                      << " for (Run " << run << ", Subrun " << sub << ", Event " << evt << ")" << std::endl;
        }
    }

    void SetHistogramStyle(TH2F* hist) {
        hist->GetXaxis()->SetTitle("Local Drift Time");
        hist->GetYaxis()->SetTitle("Local Wire Coordinate");

        // Apply styles consistent with InitialiseCommonStyle
        //hist->GetXaxis()->SetTitleFont(font_style_);
        //hist->GetYaxis()->SetTitleFont(font_style_);
        //hist->GetXaxis()->SetTitleSize(0.04f);
        //hist->GetYaxis()->SetTitleSize(0.04f);

        //hist->GetXaxis()->SetLabelFont(font_style_);
        //hist->GetYaxis()->SetLabelFont(font_style_);
        //hist->GetXaxis()->SetLabelSize(0.035f);
        //hist->GetYaxis()->SetLabelSize(0.035f);
        
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
    }

    std::tuple<int, int, int> GetEventIdentifiers(const std::string& sample_key) {
        if (use_specific_event_) {
            return {specific_run_, specific_subrun_, specific_event_};
        } else {
            auto it = dataframes_dict_.find(sample_key);
            if (it == dataframes_dict_.end()) {
                throw std::runtime_error("Sample key not found for random event selection: " + sample_key);
            }
            const auto& rnodes_vec = it->second.second; 

            if (rnodes_vec.empty()) {
                throw std::runtime_error("No RNodes found for sample during random event selection: " + sample_key);
            }
            
            auto rnode = rnodes_vec[0]; 
            
            ULong64_t count = rnode.Count().GetValue();
            if (count == 0) {
                throw std::runtime_error("No events in the first RNode for sample (random selection): " + sample_key);
            }
            int random_index = rand_gen_.Integer(count); 

            auto run_vec_all_events    = rnode.Take<int>("run").GetValue();
            auto subrun_vec_all_events = rnode.Take<int>("sub").GetValue();
            auto event_vec_all_events  = rnode.Take<int>("evt").GetValue();
            
            if (random_index >= static_cast<int>(run_vec_all_events.size()) ||
                random_index >= static_cast<int>(subrun_vec_all_events.size()) ||
                random_index >= static_cast<int>(event_vec_all_events.size())) {
                 throw std::runtime_error("Random index " + std::to_string(random_index) +
                                          " out of bounds after Take. Max size: run=" + std::to_string(run_vec_all_events.size()) +
                                          ", sub=" + std::to_string(subrun_vec_all_events.size()) +
                                          ", evt=" + std::to_string(event_vec_all_events.size()) +
                                          ". RDataFrame logic error in GetEventIdentifiers.");
            }
            return {run_vec_all_events[random_index], subrun_vec_all_events[random_index], event_vec_all_events[random_index]};
        }
    }
};

} 
#endif
