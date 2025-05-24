// ModelInferer.h
#ifndef MODELINFERER_H
#define MODELINFERER_H

#ifdef ClassDef 
#pragma push_macro("ClassDef") 
#undef ClassDef               
#define ROOT_CLASSDEF_SAVED      
#endif

#include <torch/script.h> 

#ifdef ROOT_CLASSDEF_SAVED
#pragma pop_macro("ClassDef")
#undef ROOT_CLASSDEF_SAVED
#endif

#include <vector>
#include <string>
#include <algorithm>
#include <numeric>   
#include <iostream>  
#include <cmath>     
#include <iomanip>   

const int IMG_HEIGHT = 512;
const int IMG_WIDTH = 512;
const int EMBEDDING_DIM = 128;
const int CLASSIFIER_INPUT_CHANNELS = 1;
const int CLASSIFIER_MAP_H = 128;
const int CLASSIFIER_MAP_W = 1;

namespace AnalysisFramework {

class ModelInferer {
public:
    ModelInferer(const std::string& encoder_path, const std::string& classifier_path)
        : device_(torch::kCPU) 
    {
        try {
            std::cout << "Using LibTorch device: CPU" << std::endl;

            encoder_module_ = torch::jit::load(encoder_path, device_);
            encoder_module_.eval();

            classifier_module_ = torch::jit::load(classifier_path, device_);
            classifier_module_.eval();
            std::cout << "TorchScript models loaded successfully." << std::endl;
        } catch (const c10::Error& e) {
            std::cerr << "Error loading TorchScript models: " << e.what() << std::endl;
            throw;
        }
    }

    template<typename T>
    torch::Tensor preprocess_single_view(const ROOT::VecOps::RVec<T>& view_data, int target_height, int target_width) {
        long target_len = static_cast<long>(target_height) * target_width;
        std::vector<float> processed_plane_vec(target_len);
        long current_len = view_data.size();

        if (target_len == 0 && current_len == 0) {
             return torch::empty({0}, torch::kFloat).reshape({1, 1, 0, 0});
        }
        if (target_len > 0 && current_len == 0) {
             std::fill(processed_plane_vec.begin(), processed_plane_vec.end(), 0.0f);
        } else if (target_len == 0 && current_len > 0) {
            return torch::empty({0}, torch::kFloat).reshape({1, 1, 0, 0});
        } else if (target_len > 0) {
            if (current_len == target_len) {
                std::transform(view_data.begin(), view_data.end(), processed_plane_vec.begin(),
                               [](T val){ return static_cast<float>(val); });
            } else if (current_len > target_len) {
                std::transform(view_data.begin(), view_data.begin() + target_len, processed_plane_vec.begin(),
                               [](T val){ return static_cast<float>(val); });
            } else {
                std::transform(view_data.begin(), view_data.end(), processed_plane_vec.begin(),
                               [](T val){ return static_cast<float>(val); });
                if (target_len > current_len) {
                   std::fill(processed_plane_vec.begin() + current_len, processed_plane_vec.end(), 0.0f);
                }
            }
        } else {
             return torch::empty({0}, torch::kFloat).reshape({1, 1, 0, 0});
        }

        if (target_len > 0 && !processed_plane_vec.empty()) {
            float min_val_img = processed_plane_vec[0];
            float max_val_img = processed_plane_vec[0];
            min_val_img = *std::min_element(processed_plane_vec.begin(), processed_plane_vec.end());
            max_val_img = *std::max_element(processed_plane_vec.begin(), processed_plane_vec.end());

            if (max_val_img > min_val_img) {
                for (long i = 0; i < target_len; ++i) {
                    processed_plane_vec[i] = (processed_plane_vec[i] - min_val_img) / (max_val_img - min_val_img);
                }
            } else if (max_val_img == min_val_img && min_val_img != 0.0f) {
                std::fill(processed_plane_vec.begin(), processed_plane_vec.end(), 1.0f);
            } else {
                std::fill(processed_plane_vec.begin(), processed_plane_vec.end(), 0.0f);
            }
        } else if (target_len > 0) {
             std::fill(processed_plane_vec.begin(), processed_plane_vec.end(), 0.0f);
        }

        return torch::from_blob(processed_plane_vec.data(), {1, target_height, target_width}, torch::kFloat).clone();
    }

    template<typename T_RAW, typename T_RECO>
    float get_score_for_plane(const ROOT::VecOps::RVec<T_RAW>& raw_view_data,
                              const ROOT::VecOps::RVec<T_RECO>& reco_view_data) {
        torch::NoGradGuard no_grad;

        torch::Tensor raw_tensor = preprocess_single_view(raw_view_data, IMG_HEIGHT, IMG_WIDTH).to(device_);
        torch::Tensor reco_tensor = preprocess_single_view(reco_view_data, IMG_HEIGHT, IMG_WIDTH).to(device_);

        torch::Tensor encoder_input = torch::cat({raw_tensor, reco_tensor}, 0).unsqueeze(0);

        std::vector<torch::jit::IValue> encoder_inputs_vec;
        encoder_inputs_vec.push_back(encoder_input);
        torch::Tensor embedding = encoder_module_.forward(encoder_inputs_vec).toTensor();

        torch::Tensor classifier_input = embedding.reshape({1, CLASSIFIER_INPUT_CHANNELS, CLASSIFIER_MAP_H, CLASSIFIER_MAP_W});

        std::vector<torch::jit::IValue> classifier_inputs_vec;
        classifier_inputs_vec.push_back(classifier_input);
        torch::Tensor logit = classifier_module_.forward(classifier_inputs_vec).toTensor();

        float score = 1.0f / (1.0f + std::exp(-logit.item<float>()));
        return score;
    }

    ROOT::VecOps::RVec<float> get_all_plane_scores(
        const ROOT::VecOps::RVec<float>& raw_u, const ROOT::VecOps::RVec<int>& reco_u,
        const ROOT::VecOps::RVec<float>& raw_v, const ROOT::VecOps::RVec<int>& reco_v,
        const ROOT::VecOps::RVec<float>& raw_w, const ROOT::VecOps::RVec<int>& reco_w) {

        float score_u = get_score_for_plane<float, int>(raw_u, reco_u);
        float score_v = get_score_for_plane<float, int>(raw_v, reco_v);
        float score_w = get_score_for_plane<float, int>(raw_w, reco_w);

        ROOT::VecOps::RVec<float> scores_vec;
        scores_vec.push_back(score_u);
        scores_vec.push_back(score_v);
        scores_vec.push_back(score_w);

        static long event_count_for_print = 0; // Counter for events processed by this function
        const long print_frequency = 1; // Print for every N events. Start with 1 for debugging.

        if (event_count_for_print % print_frequency == 0) {
            std::cout << "[ModelInferer DEBUG] Event #" << event_count_for_print
                      << " - Scores (U, V, W): ["
                      << std::fixed << std::setprecision(4) << score_u << ", "
                      << std::fixed << std::setprecision(4) << score_v << ", "
                      << std::fixed << std::setprecision(4) << score_w << "]"
                      << std::endl; 
                    }
        event_count_for_print++;

        return scores_vec;
    }

private:
    torch::jit::script::Module encoder_module_;
    torch::jit::script::Module classifier_module_;
    torch::Device device_;
};

} // namespace AnalysisFramework
#endif // MODELINFERER_H