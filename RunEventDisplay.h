#ifndef RUNEVENTDISPLAY_H
#define RUNEVENTDISPLAY_H

#include "DataLoader.h"
#include <string>
#include <vector>
#include <TH2F.h>
#include <TCanvas.h>
#include <TRandom3.h>
#include <ROOT/RDataFrame.hxx>
#include <iostream>

namespace AnalysisFramework {

class RunEventDisplay {
public:
    // Constructor with optional run, subrun, event numbers
    RunEventDisplay(const DataLoader::DataFramesDict& dataframes_dict, int img_size, const std::string& output_dir,
                    int run = -1, int subrun = -1, int event = -1)
        : dataframes_dict_(dataframes_dict), img_size_(img_size), output_dir_(output_dir),
          specific_run_(run), specific_subrun_(subrun), specific_event_(event),
          use_specific_event_(run >= 0 && subrun >= 0 && event >= 0), rand_gen_(0) {}

    // Visualize reconstructed input images
    void VisualiseInput(const std::string& sample_key, TCanvas* canvas) {
        auto [run, subrun, event] = GetEventIdentifiers(sample_key);
        auto it = dataframes_dict_.find(sample_key);
        if (it == dataframes_dict_.end()) {
            throw std::runtime_error("Sample key not found: " + sample_key);
        }
        const auto& [sample_type, rnodes, file_paths] = it->second;
        for (size_t i = 0; i < rnodes.size(); ++i) {
            auto& rnode = rnodes[i];
            auto filtered_df = rnode.Filter(Form("run == %d && subrun == %d && event == %d", run, subrun, event));
            if (filtered_df.Count().GetValue() > 0) {
                auto wire_images = filtered_df.Take<std::vector<std::vector<float>>>("wire_input_plane_images").GetValue();
                if (wire_images.size() != 1) {
                    throw std::runtime_error("Expected one event, but got " + std::to_string(wire_images.size()));
                }
                const auto& planes = wire_images[0];
                if (planes.size() != 3) {
                    throw std::runtime_error("Expected three planes, but got " + std::to_string(planes.size()));
                }
                canvas->Divide(3, 1);
                for (int plane = 0; plane < 3; ++plane) {
                    const auto& plane_data = planes[plane];
                    if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
                        throw std::runtime_error("Image size mismatch for plane " + plane_names_[plane]);
                    }
                    std::string hist_name = "h_input_" + plane_names_[plane];
                    std::string title = "Plane " + plane_names_[plane] + " Input (Run " + std::to_string(run) +
                                        ", Subrun " + std::to_string(subrun) + ", Event " + std::to_string(event) + ")";
                    TH2F* h_input = new TH2F(hist_name.c_str(), title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
                    float threshold = 10.0;
                    float min_value = 10.0;
                    float max_value = min_value;
                    for (int i = 0; i < img_size_; ++i) {
                        for (int j = 0; j < img_size_; ++j) {
                            float value = plane_data[j * img_size_ + i];
                            if (value > threshold) {
                                h_input->SetBinContent(i + 1, j + 1, value);
                                if (value > max_value) max_value = value;
                            } else {
                                h_input->SetBinContent(i + 1, j + 1, min_value);
                            }
                        }
                    }
                    SetHistogramStyle(h_input);
                    h_input->SetMinimum(min_value);
                    h_input->SetMaximum(max_value);
                    canvas->cd(plane + 1);
                    h_input->Draw("COL");
                    delete h_input; // Clean up
                }
                return;
            }
        }
        std::cerr << "Event not found in sample: " << sample_key << " for run " << run << ", subrun " << subrun << ", event " << event << std::endl;
    }

    // Visualize reconstructed data (similar to input for now, can be extended)
    void VisualiseReco(const std::string& sample_key, TCanvas* canvas) {
        auto [run, subrun, event] = GetEventIdentifiers(sample_key);
        auto it = dataframes_dict_.find(sample_key);
        if (it == dataframes_dict_.end()) {
            throw std::runtime_error("Sample key not found: " + sample_key);
        }
        const auto& [sample_type, rnodes, file_paths] = it->second;
        for (size_t i = 0; i < rnodes.size(); ++i) {
            auto& rnode = rnodes[i];
            auto filtered_df = rnode.Filter(Form("run == %d && subrun == %d && event == %d", run, subrun, event));
            if (filtered_df.Count().GetValue() > 0) {
                auto wire_images = filtered_df.Take<std::vector<std::vector<float>>>("wire_input_plane_images").GetValue();
                if (wire_images.size() != 1) {
                    throw std::runtime_error("Expected one event, but got " + std::to_string(wire_images.size()));
                }
                const auto& planes = wire_images[0];
                if (planes.size() != 3) {
                    throw std::runtime_error("Expected three planes, but got " + std::to_string(planes.size()));
                }
                canvas->Divide(3, 1);
                for (int plane = 0; plane < 3; ++plane) {
                    const auto& plane_data = planes[plane];
                    if (plane_data.size() != static_cast<size_t>(img_size_ * img_size_)) {
                        throw std::runtime_error("Image size mismatch for plane " + plane_names_[plane]);
                    }
                    std::string hist_name = "h_reco_" + plane_names_[plane];
                    std::string title = "Plane " + plane_names_[plane] + " Reco (Run " + std::to_string(run) +
                                        ", Subrun " + std::to_string(subrun) + ", Event " + std::to_string(event) + ")";
                    TH2F* h_reco = new TH2F(hist_name.c_str(), title.c_str(), img_size_, 0, img_size_, img_size_, 0, img_size_);
                    float threshold = 10.0;
                    float min_value = 10.0;
                    float max_value = min_value;
                    for (int i = 0; i < img_size_; ++i) {
                        for (int j = 0; j < img_size_; ++j) {
                            float value = plane_data[j * img_size_ + i];
                            if (value > threshold) {
                                h_reco->SetBinContent(i + 1, j + 1, value);
                                if (value > max_value) max_value = value;
                            } else {
                                h_reco->SetBinContent(i + 1, j + 1, min_value);
                            }
                        }
                    }
                    SetHistogramStyle(h_reco);
                    h_reco->SetMinimum(min_value);
                    h_reco->SetMaximum(max_value);
                    canvas->cd(plane + 1);
                    h_reco->Draw("COL");
                    delete h_reco; // Clean up
                }
                return;
            }
        }
        std::cerr << "Event not found in sample: " << sample_key << " for run " << run << ", subrun " << subrun << ", event " << event << std::endl;
    }

    // Visualize truth-level information (currently prints to console, can be extended to canvas)
    void VisualiseTruth(const std::string& sample_key, TCanvas* canvas) {
        auto [run, subrun, event] = GetEventIdentifiers(sample_key);
        auto it = dataframes_dict_.find(sample_key);
        if (it == dataframes_dict_.end()) {
            throw std::runtime_error("Sample key not found: " + sample_key);
        }
        const auto& [sample_type, rnodes, file_paths] = it->second;
        for (size_t i = 0; i < rnodes.size(); ++i) {
            auto& rnode = rnodes[i];
            auto filtered_df = rnode.Filter(Form("run == %d && subrun == %d && event == %d", run, subrun, event));
            if (filtered_df.Count().GetValue() > 0) {
                auto nu_e = filtered_df.Take<float>("nu_e").GetValue();
                if (nu_e.size() == 1) {
                    std::cout << "True Neutrino Energy: " << nu_e[0] << " GeV" << std::endl;
                    // Example: Add to canvas as text (requires ROOT text objects, simplified here)
                    canvas->cd();
                    canvas->Clear();
                    std::string text = "True Neutrino Energy: " + std::to_string(nu_e[0]) + " GeV";
                    TText* t = new TText(0.1, 0.5, text.c_str());
                    t->SetTextSize(0.05);
                    t->Draw();
                    delete t; // Clean up
                }
                return;
            }
        }
        std::cerr << "Event not found in sample: " << sample_key << " for run " << run << ", subrun " << subrun << ", event " << event << std::endl;
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

    // Helper to set histogram style
    void SetHistogramStyle(TH2F* hist) {
        hist->GetXaxis()->SetTitle("Local Drift Time");
        hist->GetYaxis()->SetTitle("Local Wire Coordinate");
        hist->GetXaxis()->SetTitleOffset(1.0);
        hist->GetYaxis()->SetTitleOffset(1.0);
        hist->GetXaxis()->SetLabelSize(0.03);
        hist->GetYaxis()->SetLabelSize(0.03);
        hist->GetXaxis()->SetTitleSize(0.03);
        hist->GetYaxis()->SetTitleSize(0.03);
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

    // Helper to get event identifiers (specific or random)
    std::tuple<int, int, int> GetEventIdentifiers(const std::string& sample_key) {
        if (use_specific_event_) {
            return {specific_run_, specific_subrun_, specific_event_};
        } else {
            auto it = dataframes_dict_.find(sample_key);
            if (it == dataframes_dict_.end()) {
                throw std::runtime_error("Sample key not found: " + sample_key);
            }
            const auto& [sample_type, rnodes, file_paths] = it->second;
            if (rnodes.empty()) {
                throw std::runtime_error("No RNodes found for sample: " + sample_key);
            }
            auto& rnode = rnodes[0]; // Use first RNode for simplicity
            auto count = rnode.Count().GetValue();
            if (count == 0) {
                throw std::runtime_error("No events in the RNode for sample: " + sample_key);
            }
            int random_index = rand_gen_.Integer(count);
            auto run_vec = rnode.Take<int>("run").GetValue();
            auto subrun_vec = rnode.Take<int>("subrun").GetValue();
            auto event_vec = rnode.Take<int>("event").GetValue();
            return {run_vec[random_index], subrun_vec[random_index], event_vec[random_index]};
        }
    }
};

} // namespace AnalysisFramework

#endif